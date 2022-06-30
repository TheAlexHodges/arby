//
// Created by ahodges on 23/05/22.
//

#ifndef ARBY_ARBY_BINANCE_AGGREGATE_BOOK_FEED_IMPL_HPP
#define ARBY_ARBY_BINANCE_AGGREGATE_BOOK_FEED_IMPL_HPP

#include "connector.hpp"
#include "protocol/event.hpp"
#include "rest.hpp"
#include "trading/aggregate_book_feed.hpp"
#include "util/async_sleep.hpp"

#include <queue>

namespace arby::binance::detail
{
WISE_ENUM(agg_book_state, stale, building, fresh)

struct aggregate_book_feed_impl
: std::enable_shared_from_this< aggregate_book_feed_impl >
, util::has_executor_base
, trading::implement_aggregate_book_feed< aggregate_book_feed_impl >
{
    aggregate_book_feed_impl(std::shared_ptr< connector > conn, std::shared_ptr< rest_context > rest, std::string trading_pair)
    : util::has_executor_base(conn->get_executor())
    , conn_(std::move(conn))
    , rest_(std::move(rest))
    , trading_pair_(std::move(trading_pair))
    {
        connection_condition_.state = trading::feed_state::not_ready;
        connection_condition_.errors.push_back("connection state unknown");
        book_condition_.state = trading::feed_state::not_ready;
        book_condition_.errors.push_back("no book");
    }

    constexpr static std::string_view
    classname()
    {
        return "binance::aggregate_book_feed";
    }

    void
    start()
    {
        asio::co_spawn(
            get_executor(),
            [lifetime = shared_from_this()]() -> asio::awaitable< void > { co_await lifetime->run(); },
            asio::detached);
    }

  private:
    asio::awaitable< void >
    run()
    {
        // Subscribe to connection state
        auto [conn_state_conn, conn_id, connstate] = co_await conn_->watch_connection_state(
            [weak = weak_from_this()](std::uint64_t pconn_id, connection_state const state)
            {
                if (auto self = weak.lock())
                    asio::dispatch(self->get_executor(),
                                   [self = std::move(self), state, pconn_id]() { self->handle_conn_state(pconn_id, state); });
            });
        conn_state_conn_ = std::move(conn_state_conn);
        handle_conn_state(conn_id, connstate);

        // Subscribe to all messages from connection
        auto message_conn = co_await conn_->watch_messages(
            [weak = weak_from_this()](std::uint64_t pconn_id, std::shared_ptr< connector::inbound_message_type const > message)
            {
                if (auto self = weak.lock())
                    asio::dispatch(self->get_executor(),
                                   [self = std::move(self), message = std::move(message), pconn_id]() mutable
                                   { self->handle_message(pconn_id, std::move(message)); });
            });
        message_conn_ = std::move(message_conn);
        agg_book_     = trading::aggregate_book();
    }

    void
    handle_conn_state(std::uint64_t conn_id, connection_state const state)
    {
        spdlog::debug("{}::{}: received conn state", classname(), __func__);
        assert(conn_id <= conn_id_ + 1);   // Checking we don't skip IDs

        if (conn_id < conn_id_)   // Checking we don't handle old states
        {
            spdlog::error("{}::{}: received out-of-order conn state: []-{}", classname(), __func__, conn_id, state);
            return;
        }

        if (conn_id > conn_id_ && connection_condition_.state == trading::good)   // New ID before our connection was made stale
        {
            spdlog::error("{}::{}: received out-of-order conn state: []-{}", classname(), __func__, conn_id, state);
            assert(false); // TODO handle
        }

        if (state.up())
        {
            assert(connection_condition_.state >= trading::feed_state::stale);

            conn_id_ = conn_id;
            connection_condition_.reset(trading::feed_state::good);
            book_condition_.merge(trading::feed_state::not_ready);
            asio::co_spawn(
                get_executor(),
                [self = shared_from_this(), conn_id = conn_id_]() -> asio::awaitable< void >
                { co_await self->build_agg_book(conn_id); },
                asio::detached);
        }
        else
        {
            connection_condition_.reset(trading::feed_state::stale);
            book_condition_.merge(trading::feed_state::stale);

            // Clear the book and send an update immediately - subscribers need to know about updates as soon as possible.
            agg_book_.clear();
            update();
        }
    }

    asio::awaitable< void >
    build_agg_book(std::uint64_t conn_id)
    {
        auto bad_conn = [&]() {   // Connection has changed since this coro was instantiated.
            return conn_id != conn_id_ || connection_condition_.state != trading::good;
        };

        if (bad_conn())
            co_return;
        spdlog::debug("{}::{}: fetching book snapshot", classname(), __func__);
        auto [msg, ec] = co_await rest_->v3_depth(arby::trading::spot_market_key { .base = "BTC", .term = "USDT" }, 5000);
        if (ec)
        {
            assert(false);   // TODO
        }

        if (bad_conn())
            co_return;

        spdlog::debug("{}::{}: building book", classname(), __func__);
        // Build the agg book
        for (auto &bid : msg.bids)
        {
            agg_book_.bids.add(bid);
        }
        for (auto &ask : msg.asks)
        {
            agg_book_.asks.add(ask);
        }
        agg_book_last_id_ = msg.lastUpdateId;
        // fmt::print("Agg Book from snapshot:\n{}\n", agg_book_);

        // Apply all queued updates
        while (queued_events_.size())
        {
            handle_depth_update(queued_events_.front());
            queued_events_.pop();
        }

        book_condition_.state = trading::good;
        update();
    }

    void
    handle_message(std::uint64_t conn_id, std::shared_ptr< connector::inbound_message_type const > message)
    {
        if (conn_id > conn_id_)
        {
            spdlog::debug("discarding event from old connection");
            return;
        }

        assert(conn_id == conn_id_);

        if (connection_condition_.state != trading::good)
        {
            // Connection in bad state, discard event
            spdlog::debug("discarding event because connection is bad");
            return;
        }

        protocol::events event;
        {   // parse event message
            auto start = std::chrono::steady_clock::now();
            event      = protocol::parse_event(message->object());
            auto diff  = std::chrono::steady_clock::now() - start;
            spdlog::debug("event parse took: {}", diff);
        }

        auto depth_update = boost::variant2::get_if< protocol::depth_update >(&event);
        if (!depth_update)
        {
            spdlog::error("didn't receive a depth update");
            return;
        }
        if (book_condition_.state != trading::good)
        {
            assert(book_condition_.state == trading::not_ready);
            queued_events_.push(std::move(*depth_update));
            spdlog::debug("depth update queued - {}", queued_events_.size());
            return;
        }
        handle_depth_update(*depth_update);
        update();
    }

    void
    handle_depth_update(protocol::depth_update const &update)
    {
        // Discard if old
        if (update.u <= agg_book_last_id_)
        {
            spdlog::debug("dropped old update");
            return;
        }

        if (!(update.U <= agg_book_last_id_ + 1 && update.u >= agg_book_last_id_ + 1))
        {
            assert(false);
        }

        auto start = std::chrono::steady_clock::now();   // Time the overall latency
        // bid
        for (auto &step : update.b)
        {
            if (step.depth.is_zero())
                agg_book_.bids.remove(step);   // Remove
            else
                agg_book_.bids.modify(step);   // Modify
        }

        // ask
        for (auto &step : update.a)
        {
            if (step.depth.is_zero())
                agg_book_.asks.remove(step);   // Remove
            else
                agg_book_.asks.modify(step);   // Modify
        }
        agg_book_last_id_   = update.u;
        agg_book_.timestamp = trading::timestamp_type { std::chrono::milliseconds { update.E } };

        auto diff    = std::chrono::steady_clock::now() - start;
        auto updates = update.a.size() + update.b.size();
        total_updates_ += updates;
        total_duration_ += diff;
        spdlog::debug("update agg book latency: TOTAL[{}], PER-UPDATE[{}], AVERAGE[{}]",
                      diff,
                      diff / updates,
                      total_duration_ / total_updates_);
        // fmt::print("Agg Book:\n{}\n", agg_book_);
    }

    void
    update()
    {
        auto snap = new_aggregate_book_snapshot();
        snap->condition.reset(trading::good);
        snap->condition.merge(connection_condition_);
        snap->condition.merge(book_condition_);
        snap->book = agg_book_;
        update_snapshot(std::move(snap));
    }

  private:
    util::cross_executor_connection message_conn_;
    util::cross_executor_connection conn_state_conn_;

    std::shared_ptr< connector > conn_;
    std::uint64_t                conn_id_;
    trading::feed_condition      connection_condition_;

    std::shared_ptr< rest_context > rest_;
    std::string                     trading_pair_;

    // Master agg book
    trading::aggregate_book                    agg_book_;
    std::uint64_t                              agg_book_last_id_;
    trading::feed_condition                    book_condition_;
    std::queue< protocol::depth_update const > queued_events_;

    // Testing
    std::size_t                         total_updates_;
    std::chrono::steady_clock::duration total_duration_;
};
}   // namespace arby::binance::detail

#endif   // ARBY_ARBY_BINANCE_AGGREGATE_BOOK_FEED_IMPL_HPP

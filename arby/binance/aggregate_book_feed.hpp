//
// Created by ahodges on 16/05/22.
//

#ifndef ARBY_ARBY_BINANCE_AGGREGATE_BOOK_FEED_HPP
#define ARBY_ARBY_BINANCE_AGGREGATE_BOOK_FEED_HPP

#include "connector.hpp"
#include "protocol/event.hpp"
#include "rest.hpp"
#include "trading/aggregate_book_feed.hpp"
#include "util/async_sleep.hpp"

namespace arby::binance
{

namespace detail
{
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
        fmt::print("called start\n");
        // Subscribe to all messages from connection
        auto conn = co_await conn_->watch_messages(
            [weak = weak_from_this()](std::shared_ptr< connector::inbound_message_type const > message)
            {
                if (auto self = weak.lock())
                    asio::dispatch(self->get_executor(),
                                   [self = std::move(self), message = std::move(message)]() mutable
                                   {
                                       self->handle_message(std::move(message));
                                   });
            });
        message_conn_ = std::move(conn);
        order_book_   = trading::aggregate_book();

        /*
        while (1) {
            auto result = co_await rest_->get("v3/depth?symbol=BTCUSDT&limit=1000");
            if (result.second) {

            }
            co_await util::async_sleep(std::chrono::seconds(5));
        }
        // Subscribe to connection state
        auto [conn, connstate] = co_await conn_->watch_connection_state(
            [weak = weak_from_this()](connection_state state) {

            });
            */
    }

    void
    subscribe()
    {
        auto request =
            json::value({ { "method", "SUBSCRIBE" }, { "params", { "btcusdt@aggTrade", "btcusdt@depth" } }, { "id", 1 } });
        conn_->send(json::serialize(request));
    }

    void
    handle_message(std::shared_ptr< connector::inbound_message_type const > message)
    {
        // parse event message
        protocol::events event;

        {
            auto start = std::chrono::steady_clock::now();
            event      = protocol::parse_event(message->object());
            auto diff  = std::chrono::steady_clock::now() - start;
            spdlog::debug("event parse took: {}", diff);
        }

        auto depth_update = boost::variant2::get_if< protocol::depth_update >(&event);
        if (depth_update)
            _handle_depth_update(depth_update);
        else
            spdlog::error("didn't receive a depth update");
    }

    void _handle_depth_update(protocol::depth_update * update) {
        auto start = std::chrono::steady_clock::now(); // Time the overall latency
        // bid
        for (auto &step : update->b)
        {
            if (step.depth.is_zero())
                order_book_.bids.remove(step);   // Remove
            else
                order_book_.bids.modify(step);   // Modify
        }

        // ask
        for (auto &step : update->a)
        {
            if (step.depth.is_zero())
                order_book_.asks.remove(step);   // Remove
            else
                order_book_.asks.modify(step);   // Modify
        }
        auto diff  = std::chrono::steady_clock::now() - start;
        spdlog::debug("update agg book latency: {}", diff);
        fmt::print("Agg Book:\n{}\n",order_book_);
    }

  private:
    util::cross_executor_connection message_conn_;

    std::shared_ptr< connector >    conn_;
    std::shared_ptr< rest_context > rest_;
    std::string                     trading_pair_;

    // Master order book
    trading::aggregate_book order_book_;
};
}   // namespace detail

struct aggregate_book_feed
{
    using impl_class = detail::aggregate_book_feed_impl;
    using impl_type  = std::shared_ptr< impl_class >;

    aggregate_book_feed(std::shared_ptr< connector > conn, std::shared_ptr< rest_context > rest, std::string trading_pair)
    : impl_(std::make_shared< impl_class >(std::move(conn), std::move(rest), std::move(trading_pair)))
    {
        impl_->start();
    }

  private:
    impl_type impl_;
};

}   // namespace arby::binance

#endif   // ARBY_ARBY_BINANCE_AGGREGATE_BOOK_FEED_HPP

//
// Created by ahodges on 26/05/22.
//

#ifndef ARBY_ARBY_STRATAGY_ARBITRAGE_MONITOR_HPP
#define ARBY_ARBY_STRATAGY_ARBITRAGE_MONITOR_HPP

#include "binance/aggregate_book_feed.hpp"
#include "config/asio.hpp"
#include "util/cross_executor_connection.hpp"

namespace arby::strategy::arbitrage
{

namespace detail
{

struct opportunity_monitor_impl
: util::has_executor_base
, std::enable_shared_from_this< opportunity_monitor_impl >
{
    using binance_feed = std::shared_ptr< binance::aggregate_book_feed >;
    using snapshot_type = std::shared_ptr< trading::aggregate_book_snapshot const >;


    opportunity_monitor_impl(asio::any_io_executor exec, binance_feed bfeed)
    : util::has_executor_base(std::move(exec))
    , binance_snapshot_feed_(std::move(bfeed))
    {
    }

    void
    start()
    {
        asio::co_spawn(
            get_executor(), [self = shared_from_this()]() -> asio::awaitable< void > { co_await self->run(); }, asio::detached);
    }

  private:
    asio::awaitable< void >
    run()
    {
        // Subscribe to binance agg book feed

        auto res = co_await binance_snapshot_feed_->subscribe([weak=weak_from_this()](snapshot_type snapshot){
            if (auto strong = weak.lock(); strong) {
                
            }
        })

        co_await util::async_sleep(std::chrono::seconds(10));
    }

    void
    on_binance_snapshot(snapshot_type snapshot)
    {
    }

  private:
    binance_feed binance_snapshot_feed_;
};
}   // namespace detail

// opportunity
struct opportunity_monitor
{
    using impl_class = detail::opportunity_monitor_impl;
    using impl_type  = std::shared_ptr< impl_class >;

    asio::any_io_executor
    get_executor()
    {
        return impl_->get_executor();
    }

  private:
    impl_type impl_;
};

}   // namespace arby::strategy::arbitrage

#endif   // ARBY_ARBY_STRATAGY_ARBITRAGE_MONITOR_HPP

//
// Created by ahodges on 16/05/22.
//

#ifndef ARBY_ARBY_BINANCE_AGGREGATE_BOOK_FEED_HPP
#define ARBY_ARBY_BINANCE_AGGREGATE_BOOK_FEED_HPP

#include "aggregate_book_feed_impl.hpp"

namespace arby::binance
{

namespace detail
{

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

    asio::awaitable< trading::aggregate_book_feed_iface::subscribe_result >
    subscribe(trading::aggregate_book_feed_iface::snapshot_slot slot)
    {
        co_return co_await asioex::execute_on(
            impl_->get_executor(),
            [impl = impl_, slot = std::move(slot)]() -> asio::awaitable< trading::aggregate_book_feed_iface::subscribe_result >
            { co_return co_await impl->subscribe(std::move(slot)); });
    }

  private:
    impl_type impl_;
};

}   // namespace arby::binance

#endif   // ARBY_ARBY_BINANCE_AGGREGATE_BOOK_FEED_HPP

//
// Created by ahodges on 16/05/22.
//

#ifndef ARBY_ARBY_BINANCE_ORDER_BOOK_HPP
#define ARBY_ARBY_BINANCE_ORDER_BOOK_HPP

#include "connector.hpp"

namespace arby::binance
{

namespace detail
{
struct impl : std::enable_shared_from_this< impl >
{
    impl(std::shared_ptr< connector > conn, std::string trading_pair)
    : conn_(std::move(conn))
    , trading_pair_(std::move(trading_pair))
    {
        asio::dispatch([lifetime = shared_from_this()]() { lifetime->start(); });
    }


    asio::any_io_executor
    get_executor()
    {
        return conn_->get_executor();
    }

  private:
    asio::awaitable<void>
    start()
    {
        // Subscribe to connection state
        auto [conn, connstate] = co_await conn_->watch_connection_state([weak = weak_from_this()](connection_state state){

        });

    }

    void
    subscribe()
    {
        auto request =
            json::value({ { "method", "SUBSCRIBE" }, { "params", { "btcusdt@aggTrade", "btcusdt@depth" } }, { "id", 1 } });
        conn_->send(json::serialize(request));
    }

  private:
    std::shared_ptr< connector > conn_;
    std::string                  trading_pair_;
};
}   // namespace detail

struct order_book
{
    order_book(std::shared_ptr< connector > conn, std::string trading_pair)
    : impl_(std::make_shared< detail::impl >(std::move(conn), std::move(trading_pair)))
    {
    }

  private:
    std::shared_ptr< detail::impl > impl_;
};

}   // namespace arby::binance

#endif   // ARBY_ARBY_BINANCE_ORDER_BOOK_HPP

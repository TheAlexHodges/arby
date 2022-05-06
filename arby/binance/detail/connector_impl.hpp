//
// Copyright (c) 2022 Richard Hodges (hodges.r@gmail.com)
// Copyright (c) 2022 Alexander Hodges (alexander.hodges.personal@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/arby
//

#ifndef ARBY_ARBY_CONNECTOR_IMPL_HPP
#define ARBY_ARBY_CONNECTOR_IMPL_HPP

#include "asioex/helpers.hpp"
#include "config/json.hpp"
#include "config/websocket.hpp"
#include "entity/entity_base.hpp"
#include "power_trade/connection_state.hpp"
#include "util/cross_executor_connection.hpp"

namespace arby
{
namespace binance
{
namespace detail
{

struct binance_connector_args
{
    std::string host;   // "35.186.148.56",
    std::string port;   // "4321"
    std::string path;   // "/"
};

void
merge(entity::entity_key &target, binance_connector_args const &args);


struct connector_impl : entity::entity_base
//, std::enable_shared_from_this< connector_impl >
{
    using executor_type = asio::any_io_executor;
    using tcp_layer     = tcp::socket;
    using tls_layer     = asio::ssl::stream< tcp_layer >;
    using ws_stream     = websocket::stream< tls_layer >;

    connector_impl(asio::any_io_executor exec, ssl::context &ioc, binance_connector_args args);

    std::string_view
    classname() const override { return "binance::connector"; };

  private:
    void
    extend_summary(std::string &buffer) const override;

    static asio::awaitable< void >
    run(std::shared_ptr< connector_impl > self)
    {
        co_await asioex::spin();
    }

    void
    handle_start() override;
    void
    handle_stop() override;

  private:

    asio::awaitable< void >
    run_connection();

    asio::awaitable< void >
    send_loop(ws_stream &ws);

    asio::awaitable< void >
    receive_loop(ws_stream &ws);

    asio::awaitable< void >
    interruptible_connect(ws_stream &stream);

  private:
    // dependencies
    ssl::context &ssl_ctx_;

    // arguments
    binance_connector_args args_;

    // state
    std::deque< std::string > send_queue_;
    asio::steady_timer        send_cv_ { get_executor() };
    asio::cancellation_signal interrupt_connection_;
    asio::cancellation_signal stop_;
    bool                      stopped_ = false;
};

}   // namespace detail
}   // namespace binance
}   // namespace arby

#endif   // ARBY_ARBY_CONNECTOR_IMPL_HPP

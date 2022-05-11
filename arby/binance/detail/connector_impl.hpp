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
#include "connector/inbound_message.hpp"
#include "connector_traits.hpp"
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

struct connector_impl
: entity::entity_base
, connector_traits
//, std::enable_shared_from_this< connector_impl >
{
    using executor_type = asio::any_io_executor;
    using tcp_layer     = tcp::socket;
    using tls_layer     = asio::ssl::stream< tcp_layer >;
    using ws_stream     = websocket::stream< tls_layer >;

    using inbound_message_type = connector::inbound_message< beast::flat_buffer >;

    connector_impl(asio::any_io_executor exec, ssl::context &ioc, binance_connector_args args);

    std::string_view
    classname() const override
    {
        return "binance::connector";
    };

    void
    send(std::string s);
    void
    interrupt();

    // boost::signals2::connection watch_messages(json::string message_type, message_slot slot)

  private:
    void
    extend_summary(std::string &buffer) const override;
    void
    handle_start() override;
    void
    handle_stop() override;

  private:
    static asio::awaitable< void >
    run(std::shared_ptr< connector_impl > self);

    asio::awaitable< void >
    run_connection();

    asio::awaitable< void >
    send_loop(ws_stream &ws);

    asio::awaitable< void >
    receive_loop(ws_stream &ws);

    asio::awaitable< void >
    interruptible_connect(ws_stream &stream);

    bool
    handle_message(std::shared_ptr< inbound_message_type const > pmessage);

    void
    set_connection_state(error_code ec);

  private:
    // dependencies
    ssl::context &ssl_ctx_;

    // arguments
    binance_connector_args args_;

    connection_state_signal connstate_signal_;
    connection_state        connstate_ { asio::error::not_connected };

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

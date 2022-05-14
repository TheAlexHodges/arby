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
#include "connector/connector_traits.hpp"
#include "entity/entity_base.hpp"
#include "power_trade/connection_state.hpp"
#include "util/cross_executor_connection.hpp"
#include "util/signal_map.hpp"

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
, connector::connector_traits
{
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

    boost::signals2::connection
    watch_messages(json::string message_type, message_slot slot);

    boost::signals2::connection
    watch_connection_state(connection_state &current, connection_state_slot slot);

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

    using signal_map_type = util::signal_map< json::string, message_signal >;
    signal_map_type signal_map_;

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

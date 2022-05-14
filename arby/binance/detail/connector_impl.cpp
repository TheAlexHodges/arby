//
// Copyright (c) 2022 Richard Hodges (hodges.r@gmail.com)
// Copyright (c) 2022 Alexander Hodges (alexander.hodges.personal@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/arby
//

#include "connector_impl.hpp"

#include "asioex/scoped_interrupt.hpp"
#include "network/connect_ssl.hpp"
#include "util/cross_executor_connection.hpp"
#include "util/monitor.hpp"
#include "util/truncate.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/time_traits.hpp>
#include <boost/scope_exit.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <spdlog/spdlog.h>

namespace arby
{
namespace binance
{
namespace detail
{
using namespace std::literals;
void
merge(entity::entity_key &target, binance_connector_args const &args)
{
    target.set("binance.Host", args.host);
    target.set("binance.Port", args.port);
    target.set("binance.Path", args.path);
}

connector_impl::connector_impl(asio::any_io_executor exec, ssl::context &ioc, binance_connector_args args)
: entity::entity_base(std::move(exec))
, ssl_ctx_(ioc)
, args_(std::move(args))
{
}
void
connector_impl::extend_summary(std::string &buffer) const
{
}

void
connector_impl::send(std::string s)
{
    send_queue_.push_back(std::move(s));
    send_cv_.cancel();
}

void
connector_impl::interrupt()
{
    interrupt_connection_.emit(asio::cancellation_type::all);
}

void
connector_impl::handle_start()
{
    asio::co_spawn(
        get_executor(),
        [lifetime = shared_from_this()]() -> asio::awaitable< void >
        { co_await run(std::static_pointer_cast< connector_impl >(lifetime)); },
        asio::bind_cancellation_slot(stop_.slot(), asio::detached));
}

void
connector_impl::handle_stop()
{
    asio::dispatch(get_executor(),
                   [lifetime = shared_from_this()]()
                   {
                       auto self = std::static_pointer_cast< connector_impl >(lifetime);
                       fmt::print("{}::{}\n", self->classname(), "stop");
                       assert(!self->stopped_);
                       self->stopped_ = true;
                       asioex::terminate(self->stop_);
                   });
}

asio::awaitable< void >
connector_impl::run(std::shared_ptr< connector_impl > self)
{
    auto sentinel = util::monitor::record(fmt::format("{}::{}", self->classname(), __func__));

    for (; !self->stopped_;)
    {
        try
        {
            co_await self->run_connection();
        }
        catch (std::exception &e)
        {
            fmt::print("{}::{} : exception : {}", self->classname(), __func__, e.what());
        }
        if (self->stopped_)
            break;
        auto t = asio::steady_timer(self->get_executor(), 2s);
        fmt::print("{}::{} : reconnect delay...\n", self->classname(), __func__);
        co_await t.async_wait(asio::use_awaitable);
    }
}

asio::awaitable< void >
connector_impl::run_connection()
{
    auto sentinel = util::monitor::record(fmt::format("{}::{}", classname(), __func__));

    try
    {
        using asio::bind_cancellation_slot;
        using asio::co_spawn;
        using asio::use_awaitable;
        using namespace asio::experimental::awaitable_operators;

        auto ws = ws_stream(get_executor(), ssl_ctx_);

        co_await interruptible_connect(ws);

        fmt::print("{}::{} : websocket connected!\n", classname(), __func__);

        set_connection_state(error_code());
        send_cv_.cancel();

        {
            auto forward_signal    = asio::cancellation_signal();
            auto forward_interrupt = [&](std::string_view reason)
            {
                return [&, reason]
                {
                    fmt::print("{}::{} : forwarding interrupt: {}", classname(), __func__, reason);
                    ws.next_layer().next_layer().close();
                    asioex::terminate(forward_signal);
                };
            };

            auto s0 = asioex::scoped_interrupt((co_await asio::this_coro::cancellation_state).slot(), forward_interrupt("stop"));
            auto s1 = asioex::scoped_interrupt(interrupt_connection_, forward_interrupt("interrupt"));
            co_await co_spawn(
                get_executor(), send_loop(ws) || receive_loop(ws), bind_cancellation_slot(forward_signal.slot(), use_awaitable));
        }
        set_connection_state(asio::error::not_connected);
    }
    catch (boost::system::system_error &se)
    {
        set_connection_state(se.code());
        fmt::print("{}::{} : exception: {}\n", classname(), __func__, se.what());
    }
    catch (std::exception &e)
    {
        set_connection_state(asio::error::no_such_device);
        fmt::print("{}::{} : exception: {}\n", classname(), __func__, e.what());
    }
    fmt::print("{}::{} : complete\n", classname(), __func__);
}

asio::awaitable< void >
connector_impl::interruptible_connect(connector_impl::ws_stream &stream)
{
    auto sentinel = util::monitor::record(fmt::format("{}::{}", classname(), __func__));

    bool interrupted       = false;
    auto cancel_connect    = asio::cancellation_signal();
    auto forward_interrupt = [&](asio::cancellation_type type)
    {
        interrupted = true;
        cancel_connect.emit(type);
    };

    {
        auto ic_slot = interrupt_connection_.slot();
        ic_slot.assign(forward_interrupt);
        BOOST_SCOPE_EXIT_ALL(ic_slot)
        {
            ic_slot.clear();
        };

        auto my_slot = (co_await asio::this_coro::cancellation_state).slot();
        my_slot.assign(forward_interrupt);
        BOOST_SCOPE_EXIT_ALL(my_slot)
        {
            my_slot.clear();
        };

        co_await co_spawn(get_executor(),
                          network::connect(stream, args_.host, args_.port, args_.path),
                          asio::bind_cancellation_slot(cancel_connect.slot(), asio::use_awaitable));
    }
    if (interrupted)
        throw system_error(asio::error::operation_aborted, "interrupted");
}

asio::awaitable< void >
connector_impl::send_loop(ws_stream &ws)
{
    auto sentinel = util::monitor::record(fmt::format("{}::{}", classname(), __func__));

    try
    {
        for (;;)
        {
            while (connstate_.up() && send_queue_.empty())
            {
                send_cv_.expires_at(asio::steady_timer::time_point::max());
                error_code ec;
                co_await send_cv_.async_wait(asio::redirect_error(asio::use_awaitable, ec));
            }
            if (connstate_.down())
                break;
            assert(!send_queue_.empty());
            fmt::print("{}::{}: sending {}\n", classname(), __func__, send_queue_.front());
            co_await ws.async_write(asio::buffer(send_queue_.front()), asio::use_awaitable);
            send_queue_.pop_front();
        }
    }
    catch (std::exception &e)
    {
        fmt::print("{}::{}: exception: {}\n", classname(), __func__, e.what());
    }
    fmt::print("{}::{}: complete\n", classname(), __func__);
}

asio::awaitable< void >
connector_impl::receive_loop(ws_stream &ws)
{
    auto sentinel = util::monitor::record(fmt::format("{}::{}", classname(), __func__));
    try
    {
        error_code ec;
        auto       pmessage = std::make_shared< inbound_message_type >(std::size_t(10'000'000));
        for (; !ec;)
        {
            auto size = co_await ws.async_read(pmessage->prepare(), asio::redirect_error(asio::use_awaitable, ec));
            if (ec)
            {
                fmt::print("{}::{}: read error: {}\n", classname(), __func__, ec.message());
                break;
            }
            try
            {
                pmessage->commit();
                if (!handle_message(pmessage))
                    spdlog::error("{}::{}: unhandled inbound message {}", classname(), __func__, util::truncate(pmessage->view()));
            }
            catch (system_error &se)
            {
                ec = se.code();
            }
            catch (std::exception &e)
            {
                ec = asio::error::invalid_argument;
            }
        }
        if (connstate_.up())
            set_connection_state(ec);
        send_cv_.cancel();
    }
    catch (std::exception &e)
    {
        fmt::print("{}::{}: exception: {}\n", classname(), __func__, e.what());
    }
    fmt::print("{}::{}: complete\n", classname(), __func__);
}

bool
connector_impl::handle_message(std::shared_ptr< inbound_message_type const > pmessage)
{
    auto ifind = signal_map_.find(pmessage->type());
    if (ifind == signal_map_.end())
        return false;
    ifind->second(pmessage);
    return true;
}

boost::signals2::connection
connector_impl::watch_messages(json::string message_type, message_slot slot)
{
    auto &sig = signal_map_[message_type];
    return sig.connect(std::move(slot));
}

boost::signals2::connection
connector_impl::watch_connection_state(connection_state &current, connection_state_slot slot)
{
    current = connstate_;
    return connstate_signal_.connect(std::move(slot));
}

void
connector_impl::set_connection_state(error_code ec)
{
    connstate_.set(ec);
    connstate_signal_(connstate_);
}


}   // namespace detail
}   // namespace binance
}   // namespace arby
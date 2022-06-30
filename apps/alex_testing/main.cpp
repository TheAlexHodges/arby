//
// Created by ahodges on 14/05/22.
//

#include "asioex/scoped_interrupt.hpp"
#include "config.hpp"
#include "util/monitor.hpp"

#include <binance/aggregate_book_feed.hpp>
#include <binance/connector.hpp>

#include <power_trade/connector.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <spdlog/spdlog.h>

#include <termios.h>

namespace
{
struct termios original_termios;
}

namespace asio   = arby::asio;
using work_guard = asio::executor_work_guard< asio::any_io_executor >;

void
set_up_stdin()
{
    ::tcgetattr(STDIN_FILENO, &original_termios);
    auto my_termios = original_termios;
    my_termios.c_lflag &= ~(ICANON);
    std::atexit([]() { ::tcsetattr(STDIN_FILENO, TCSANOW, &original_termios); });

    //    ::cfmakeraw(&my_termios);
    ::tcsetattr(STDIN_FILENO, TCSANOW, &my_termios);
}

struct application : std::enable_shared_from_this< application >
{
    application(asio::any_io_executor exec, arby::ssl::context &sslctx)
    : exec_(exec)
    , sslctx_(sslctx)
    {
    }

    asio::awaitable< void >
    monitor_keys(std::unordered_map< char, arby::sigs::signal< void() > > &signals)
    {
        auto sentinel = arby::util::monitor::record(__func__);
        using asio::redirect_error;
        using asio::use_awaitable;

        set_up_stdin();
        auto input = asio::posix::stream_descriptor(co_await asio::this_coro::executor, ::dup(STDIN_FILENO));

        auto cstate = co_await asio::this_coro::cancellation_state;
        for (; cstate.cancelled() == asio::cancellation_type::none;)
        {
            char                      buf;
            boost::system::error_code ec;
            auto size = co_await asio::async_read(input, asio::mutable_buffer(&buf, 1), redirect_error(use_awaitable, ec));
            if (ec)
                break;
            if (!size)
                continue;
            spdlog::trace("calling signal with key: {}", buf);
            signals[buf]();
            spdlog::trace("finished calling signal");
        }
    }

    asio::awaitable< void >
    on_conn_state(std::uint64_t conn_id, arby::connection_state state)
    {
        fmt::print("new connector state: {}:{}\n", conn_id, state);
        co_return;
    }

    auto
    setup_binance()
    {
        auto rest = std::make_shared< arby::binance::rest_context >(
            exec_, arby::binance::rest_context_args { .host = "api.binance.com", .port = "443" });

        binance_conn_ = std::make_shared< arby::binance::connector >(
            exec_,
            sslctx_,
            arby::binance::detail::binance_connector_args { "stream.binance.com", "9443", "/ws/btcusdt@depth@100ms" });

        // Create order book
        agg_book_feed_ = std::make_unique< arby::binance::aggregate_book_feed >(binance_conn_, rest, "");
        agg_book_feed
    }

    auto
    setup_power_trade()
    {

    }

    asio::awaitable< void >
    run()
    {
        auto                          this_exec = co_await asio::this_coro::executor;
        arby::sigs::scoped_connection qcon0     = key_signals_['q'].connect([&] { arby::asioex::terminate(stop_monitor_); });

        setup_binance();
        auto rcon = key_signals_['i'].connect(
            [&]()
            {
                spdlog::warn("interrupt CB");
                binance_conn_->interrupt();
                spdlog::warn("interrupt CB finish");
            });

        co_await co_spawn(
            this_exec,
            [&] { return monitor_keys(key_signals_); },
            asio::bind_cancellation_slot(stop_monitor_.slot(), asio::use_awaitable));
    }

  private:
    arby::ssl::context                                      &sslctx_;
    asio::any_io_executor                                    exec_;
    std::unordered_map< char, arby::sigs::signal< void() > > key_signals_;
    asio::cancellation_signal                                stop_monitor_;

    // Binance
    std::shared_ptr< arby::binance::connector >              binance_conn_;
    std::unique_ptr< arby::binance::aggregate_book_feed >    agg_book_feed_;

    // Powertrade
    std::shared_ptr<arby::power_trade::connector> power_trade_conn_;
    std::shared_ptr<arby::power_trade::> book_feed_;
};

int
main()
{
    spdlog::set_level(spdlog::level::debug);                                               // logging level
    auto cfg = app::config::load("/home/ahodges/projects/madmongo1/arby/alex_cfg.json");   // Load config

    arby::ssl::context sslctx(arby::ssl::context_base::tls_client);
    auto               ioc = arby::asio::io_context(1);
    auto               app = application(ioc.get_executor(), sslctx);
    asio::co_spawn(ioc.get_executor(), app.run(), asio::detached);
    try
    {
        ioc.run();
    }
    catch (std::exception &ec)
    {
        fmt::print("error {}\n", ec.what());
    }
}

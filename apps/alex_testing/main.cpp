//
// Created by ahodges on 14/05/22.
//

#include <binance/aggregate_book_feed.hpp>
#include <binance/connector.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <spdlog/spdlog.h>

namespace asio = arby::asio;

using work_guard = asio::executor_work_guard< asio::any_io_executor >;

unsigned int                                          state_changes = 0;
std::shared_ptr< arby::binance::connector >           binance_conn;
std::unique_ptr< arby::binance::aggregate_book_feed > agg_book_feed;

struct application : std::enable_shared_from_this< application >
{
    asio::awaitable< void >
    on_conn_state(arby::connection_state state, work_guard &wg)
    {
        fmt::print("new connector state: {}\n", state);
        if (++state_changes == 3)
        {
            wg.reset();
            binance_conn.reset();
        }
        co_return;
    }

    asio::awaitable< void >
    run(arby::ssl::context sslctx, work_guard &wg)
    {
        auto this_exec = co_await asio::this_coro::executor;

        binance_conn = std::make_shared< arby::binance::connector >(
            this_exec,
            sslctx,
            arby::binance::detail::binance_connector_args { "stream.binance.com", "9443", "/ws/btcusdt@depth@100ms" });

        // Subscribe to connection state
        auto [conn, state] = co_await binance_conn->watch_connection_state(arby::binance::connector::connection_state_slot(
            [this_exec, &wg, this](arby::connection_state state)
            { asio::co_spawn(this_exec, on_conn_state(std::move(state), wg), asio::detached); }));
        fmt::print("Current state: {}\n", state);
        conn_ = std::move(conn);

        // Create order book
        agg_book_feed = std::make_unique< arby::binance::aggregate_book_feed >(binance_conn, nullptr, "");
    }

  private:
    arby::util::cross_executor_connection conn_;
};

int
main()
{
    /*
    auto one = std::string_view("{\"result\":null,\"id\":1}");
    auto two = std::string_view(
        "{\"stream\":\"btcusdt@aggTrade\",\"data\":{\"e\":\"aggTrade\",\"E\":1652831974503,\"s\":\"BTCUSDT\",\"a\":1178262225,\"p\":\"30443.97000000\",\"q\":\"0.00835000\",\"f\":1371480310,\"l\":1371480310,\"T\":1652831974502,\"m\":true,\"M\":true}}");
    auto vone = arby::json::parse(one);
    auto vtwo = arby::json::parse(two);
    auto foo  = arby::binance::stream_protocol::server::parse(vone);
    auto bar  = arby::binance::stream_protocol::server::parse(vtwo);
    if (boost::variant2::holds_alternative< arby::binance::stream_protocol::server::stream_response >(foo))
    {
        fmt::print("success foo\n");
    }
    if (boost::variant2::holds_alternative< arby::binance::stream_protocol::server::agg_trade_update >(bar))
    {
        fmt::print("success bar\n");
    }
    return 0;

    /*fmt::print("{}",arby::binance::stream_protocol::subscribe<std::string>(1,"btcusdc@trade"));
    return 0;
     */

    spdlog::set_level(spdlog::level::debug);

    arby::ssl::context sslctx(arby::ssl::context_base::tls_client);

    auto       ioc = arby::asio::io_context(1);
    work_guard wg  = asio::make_work_guard(asio::any_io_executor(ioc.get_executor()));
    auto       app = application();
    asio::co_spawn(ioc.get_executor(), app.run(std::move(sslctx), wg), asio::detached);
    try
    {
        ioc.run();
    }
    catch (std::exception &ec)
    {
        fmt::print("error {}\n", ec.what());
    }
}

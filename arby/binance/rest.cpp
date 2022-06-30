//
// Created by ahodges on 20/05/22.
//
#include "rest.hpp"

namespace arby::binance
{

namespace detail
{

asio::awaitable< std::tuple< http::response< http::string_body >, error_code > >
rest_context_impl::get(std::string url)
{
    using response_type = http::response< http::string_body >;
    using ret_type      = std::tuple< response_type, error_code >;
    using req_type      = http::request< http::string_body >;

    error_code ec;

    auto sslctx = ssl::context(ssl::context_base::tls_client);

    // Lookup domain
    auto resolver = tcp::resolver(get_executor());
    auto results  = co_await resolver.async_resolve(args_.host, args_.port, asio::redirect_error(asio::use_awaitable, ec));
    if (ec)
    {
        spdlog::error("{}::{}: resolve: {}", classname(), __func__, ec.what());
        co_return ret_type(response_type(), ec);
    }

    auto stream = beast::ssl_stream<beast::tcp_stream>(get_executor(),sslctx);
    stream.next_layer().expires_after(std::chrono::seconds(30));

    co_await stream.next_layer().async_connect(results, asio::redirect_error(asio::use_awaitable, ec));
    if (ec)
    {
        spdlog::error("{}::{}: connect: {}", classname(), __func__, ec.what());
        co_return ret_type(response_type(), ec);
    }
    co_await stream.async_handshake(ssl::stream_base::client,asio::redirect_error(asio::use_awaitable, ec));
    if (ec)
    {
        spdlog::error("{}::{}: handshake: {}", classname(), __func__, ec.what());
        co_return ret_type(response_type(), ec);
    }

    // Setup GET message
    auto req = req_type(http::verb::get, url, 11);
    req.set(http::field::host, args_.host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    //req.set(http::field::user_agent, "python-requests/2.27.1");
    //req.set(http::field::accept_encoding, "gzip, deflate");
    req.set(http::field::accept, "*/*");
    stream.next_layer().expires_after(std::chrono::seconds(30));

    co_await http::async_write(stream, req, asio::redirect_error(asio::use_awaitable, ec));
    if (ec)
    {
        spdlog::error("{}::{}: write: {}", __FILE__, classname(), ec.what());
        co_return ret_type(response_type(), ec);
    }

    beast::flat_buffer b;
    response_type      res;
    co_await http::async_read(stream, b, res, asio::redirect_error(asio::use_awaitable, ec));
    if (ec)
    {
        spdlog::error("{}::{}: read: {}", classname(), __func__, ec.what());
        co_return ret_type(response_type(), ec);
    }

    // Close connection
    stream.next_layer().close();
    co_return ret_type(res, error_code());
}
}   // namespace detail

}   // namespace arby::binance
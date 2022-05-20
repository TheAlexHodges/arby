//
// Created by ahodges on 18/05/22.
//

#ifndef ARBY_ARBY_BINANCE_REST_HPP
#define ARBY_ARBY_BINANCE_REST_HPP

#include "asioex/execute_on.hpp"
#include "config/format.hpp"
#include "config/http.hpp"
#include "util/cross_executor_connection.hpp"

#include <boost/beast/version.hpp>

namespace arby::binance
{
namespace detail
{

std::string
parse_url(const std::string &base, const std::string &addition)
{
    if (base.ends_with('/'))
    {
        if (addition.starts_with('/'))
        {
            return base + addition.substr(1, addition.length() - 1);
        }
        return base + addition;
    }
    if (addition.starts_with('/'))
        return base + addition;
    return base + '/' + addition;
}

struct rest_context_impl
: std::enable_shared_from_this< rest_context_impl >
, util::has_executor_base
{
    rest_context_impl(asio::any_io_executor exec)
    : util::has_executor_base(exec)
    {
    }

    constexpr static std::string_view
    classname()
    {
        return "binance::rest";
    }

    asio::awaitable< std::pair< http::response< http::dynamic_body >, error_code > >
    get(std::string url)
    {
        using response_type = http::response< http::dynamic_body >;
        using ret_type      = std::pair< response_type, error_code >;
        using req_type      = http::request< http::string_body >;

        error_code ec;

        // Lookup domain
        auto resolver = tcp::resolver(get_executor());
        auto results  = co_await resolver.async_resolve(host_, port_, asio::redirect_error(asio::use_awaitable, ec));
        if (ec)
        {
            spdlog::error("{}::{}: resolve: {}", __FILE__, classname(), ec.what());
            co_return ret_type(response_type(), ec);
        }

        auto stream = beast::tcp_stream(get_executor());
        stream.expires_after(std::chrono::seconds(30));

        co_await stream.async_connect(results, asio::redirect_error(asio::use_awaitable, ec));
        if (ec)
        {
            spdlog::error("{}::{}: connect: {}", __FILE__, classname(), ec.what());
            co_return ret_type(response_type(), ec);
        }

        // Setup GET message
        auto req = req_type(http::verb::get, parse_url(base_uri_, url), 1);
        req.set(http::field::host, host_);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        stream.expires_after(std::chrono::seconds(30));

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
            spdlog::error("{}::{}: read: {}", __FILE__, classname(), ec.what());
            co_return ret_type(response_type(), ec);
        }

        // Close connection
        stream.close();

        co_return std::make_pair(res, error_code());
    }

  private:
    const std::string host_, port_, base_uri_;
};
}   // namespace detail

struct rest_context
{
    rest_context(asio::any_io_executor exec)
    : impl_(std::make_shared< detail::rest_context_impl >(exec))
    {
    }

    asio::any_io_executor
    get_executor()
    {
        return impl_->get_executor();
    }

    asio::awaitable< std::pair< http::response< http::dynamic_body >, error_code > >
    get(std::string url)
    {
        co_return co_await asioex::execute_on(get_executor(),
                                              [lifetime = impl_, url = std::move(url)]() mutable
                                              -> asio::awaitable< std::pair< http::response< http::dynamic_body >, error_code > >
                                              { co_return co_await lifetime->get(std::move(url)); });
    }

  private:
    std::shared_ptr< detail::rest_context_impl > impl_;
};

}   // namespace arby::binance

#endif   // ARBY_ARBY_BINANCE_REST_HPP

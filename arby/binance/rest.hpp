//
// Created by ahodges on 18/05/22.
//

#ifndef ARBY_ARBY_BINANCE_REST_HPP
#define ARBY_ARBY_BINANCE_REST_HPP

#include "asioex/execute_on.hpp"
#include "config/format.hpp"
#include "config/http.hpp"
#include "protocol/http.hpp"
#include "util/cross_executor_connection.hpp"

#include <boost/beast/version.hpp>

namespace arby::binance
{

struct rest_context_args
{
    std::string host, port;
};

namespace detail
{

inline std::string
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
    rest_context_impl(asio::any_io_executor exec, rest_context_args args)
    : util::has_executor_base(exec)
    , args_(std::move(args))
    {
    }

    constexpr static std::string_view
    classname()
    {
        return "binance::rest";
    }

    asio::awaitable< std::tuple< http::response< http::string_body >, error_code > >
    get(std::string url);

  private:
    const rest_context_args args_;
};
}   // namespace detail

struct rest_context
{
    rest_context(asio::any_io_executor exec, rest_context_args args)
    : impl_(std::make_shared< detail::rest_context_impl >(exec, std::move(args)))
    {
    }

    asio::any_io_executor
    get_executor()
    {
        return impl_->get_executor();
    }

    asio::awaitable< std::tuple< protocol::http::v3depth, error_code > >
    v3_depth(trading::market_key const & key, std::size_t const limit = 5000)
    {
        assert(limit <= 5000);

        // Extract spot market key
        auto spot_key = get_if<trading::spot_market_key>(&key.as_variant());

        std::string url;
        if (limit == 0) {
            url = fmt::format("/api/v3/depth?symbol={}{}",spot_key->base,spot_key->term);
        } else {
            url = fmt::format("/api/v3/depth?symbol={}{}&limit={}",spot_key->base,spot_key->term,limit);
        }

        auto [http_resp, ec] = co_await get(url);
        if (ec)
            co_return std::make_tuple(protocol::http::v3depth{},ec);

        if (http_resp.result() != http::status::ok)
            co_return std::make_tuple(protocol::http::v3depth{},error_code(asio::error::fault));

        // OK response type, try and parse the body as json
        auto &body = http_resp.body();
        //spdlog::debug(body);
        auto v = json::parse(boost::string_view(body.data(), body.size()));

        // Attempt to parse into the protocol message
        auto pmsg = protocol::http::v3depth::parse(v);
        co_return std::make_tuple(pmsg,ec);
    }

  private:
    asio::awaitable< std::tuple< http::response< http::string_body >, error_code > >
    get(std::string url)
    {
        co_return co_await asioex::execute_on(get_executor(),
                                              [lifetime = impl_, url = std::move(url)]() mutable
                                              -> asio::awaitable< std::tuple< http::response< http::string_body >, error_code > >
                                              { co_return co_await lifetime->get(std::move(url)); });
    }

  private:
    std::shared_ptr< detail::rest_context_impl > impl_;
};

}   // namespace arby::binance

#endif   // ARBY_ARBY_BINANCE_REST_HPP

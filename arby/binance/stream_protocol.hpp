//
// Created by ahodges on 18/05/22.
//

#ifndef ARBY_ARBY_BINANCE_STREAM_PROTOCOL_HPP
#define ARBY_ARBY_BINANCE_STREAM_PROTOCOL_HPP

#include "config/json.hpp"

#include <boost/variant2.hpp>

#include <cstdarg>

namespace arby::binance::stream_protocol
{
namespace
{
constexpr auto to_price     = [](json::value const &v) { return trading::price_type(v.as_string().c_str()); };
constexpr auto to_qty       = [](json::value const &v) { return trading::qty_type(v.as_string().c_str()); };
constexpr auto to_side      = [](json::value const &v) { return *wise_enum::from_string< trading::side_type >(v.as_string()); };
constexpr auto to_timestamp = [](json::value const &v)
{
    using namespace std::chrono;

    auto nanos = nanoseconds(::atol(v.as_string().c_str()));
    return system_clock::time_point(duration_cast< system_clock::duration >(nanos));
};
constexpr auto to_opt_string = [](json::value const &v) -> std::optional< std::string >
{
    if (v.is_null())
        return std::nullopt;
    return std::optional(json::value_to< std::string >(v));
};
}   // namespace

namespace client   // Server-bound message generators
{
template < class... String >
std::string
subscribe(unsigned int id, const String &&...params)
{
    auto val = json::value({ { "method", "SUBSCRIBE" }, { "params", json::array({ params... }) }, { "id", id } });
    return serialize(val);
}
}   // namespace client

namespace server   // Client-Bound message handling
{
struct stream_response
{
    std::optional< std::string > result;
    std::uint64_t                id;
};

template < class Data >
struct stream_update
{
    std::string stream;
    Data        data;
};

struct agg_trade_data
{
    std::string         e;
    std::uint64_t       E;
    std::string         s;
    std::uint64_t       a;
    trading::price_type p;
    trading::qty_type   q;
    std::uint64_t       f;
    std::uint64_t       l;
    std::uint64_t       T;
    bool                m;
    bool                M;
};

typedef stream_update< agg_trade_data > agg_trade_update;

typedef boost::variant2::variant< stream_response, agg_trade_update > message_v;

message_v
parse(const json::value &v)
{
    // TODO optimise
    auto &o = v.as_object();
    for (auto begin = o.begin(); begin != o.end(); begin++)
    {
        auto key = begin->key();
        if (key == "result")
        {
            return stream_response { .result = to_opt_string(o.at("result")), .id = json::value_to< std::uint64_t >(o.at("id")) };
        }
        if (key == "stream" || key == "data")
        {
            auto &data = o.at("data").as_object();
            return agg_trade_update { .stream = json::value_to< std::string >(o.at("stream")),
                                      .data   = agg_trade_data { .e = json::value_to< std::string >(data.at("e")),
                                                                 .E = json::value_to< std::uint64_t >(data.at("E")),
                                                                 .s = json::value_to< std::string >(data.at("s")),
                                                                 .a = json::value_to< std::uint64_t >(data.at("a")),
                                                                 .p = to_price(data.at("p")),
                                                                 .q = to_qty(data.at("q")),
                                                                 .f = json::value_to< std::uint64_t >(data.at("f")),
                                                                 .l = json::value_to< std::uint64_t >(data.at("l")),
                                                                 .T = json::value_to< std::uint64_t >(data.at("T")),
                                                                 .m = json::value_to< bool >(data.at("m")),
                                                                 .M = json::value_to< bool >(data.at("M")) } };
        }
    }
}

}   // namespace server
}   // namespace arby::binance::stream_protocol

#endif   // ARBY_ARBY_BINANCE_STREAM_PROTOCOL_HPP

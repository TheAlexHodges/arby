//
// Created by ahodges on 19/05/22.
//

#ifndef ARBY_ARBY_BINANCE_PROTOCOL_DETAIL_JSON_CONVERSION_HPP
#define ARBY_ARBY_BINANCE_PROTOCOL_DETAIL_JSON_CONVERSION_HPP

#include "config/json.hpp"
#include "trading/types.hpp"

namespace arby::binance::protocol::detail
{

namespace json_conversion
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

}   // namespace json_conversion

}   // namespace arby::binance::protocol::detail

#endif   // ARBY_ARBY_BINANCE_PROTOCOL_DETAIL_JSON_CONVERSION_HPP

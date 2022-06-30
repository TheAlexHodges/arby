//
// Created by ahodges on 21/05/22.
//

#ifndef ARBY_ARBY_BINANCE_PROTOCOL_HTTP_HPP
#define ARBY_ARBY_BINANCE_PROTOCOL_HTTP_HPP

#include "config/json.hpp"
#include "detail/json_conversion.hpp"
#include "trading/aggregate_book.hpp"

#include <vector>

namespace arby::binance::protocol::http
{

struct v3depth
{
    using ladder_type = std::vector< trading::price_depth >;

    static v3depth
    parse(json::value const &v)
    {
        auto build_ladder = [](const json::array &arr)
        {
            using namespace detail::json_conversion;

            auto ladder = ladder_type();
            ladder.reserve(arr.size());
            for (auto &ival : arr)
            {
                auto &iarr = ival.as_array();
                ladder.emplace_back(trading::price_depth { .price = to_price(iarr.at(0)), .depth = to_qty(iarr.at(1)) });
            }
            return ladder;
        };

        auto &o = v.as_object();
        return v3depth {
            .lastUpdateId = json::value_to< std::uint64_t >(o.at("lastUpdateId")),
            .bids         = build_ladder(o.at("bids").as_array()),
            .asks         = build_ladder(o.at("bids").as_array()),
        };
    }

    std::uint64_t lastUpdateId;
    ladder_type   bids;
    ladder_type   asks;
};

}   // namespace arby::binance::protocol::http
#endif   // ARBY_ARBY_BINANCE_PROTOCOL_HTTP_HPP

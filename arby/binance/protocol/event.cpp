//
// Created by ahodges on 19/05/22.
//

#include "event.hpp"

namespace arby::binance::protocol
{
agg_trade
agg_trade::parse(const json::object &o)
{
    using namespace detail::json_conversion;
    return agg_trade { .e = json::value_to< std::string >(o.at("e")),
                       .E = json::value_to< std::uint64_t >(o.at("E")),
                       .s = json::value_to< std::string >(o.at("s")),
                       .a = json::value_to< std::uint64_t >(o.at("a")),
                       .p = to_price(o.at("p")),
                       .q = to_qty(o.at("q")),
                       .f = json::value_to< std::uint64_t >(o.at("f")),
                       .l = json::value_to< std::uint64_t >(o.at("l")),
                       .T = json::value_to< std::uint64_t >(o.at("T")),
                       .m = json::value_to< bool >(o.at("m")),
                       .M = json::value_to< bool >(o.at("M")) };
}
depth_update
depth_update::parse(const json::object &o)
{
    auto build_ladder = [](const json::array &arr)
    {
        using namespace detail::json_conversion;

        auto ladder = pl_update_list();
        ladder.reserve(arr.size());
        for (auto &ival : arr)
        {
            auto &iarr = ival.as_array();
            ladder.emplace_back(trading::price_depth{.price = to_price(iarr.at(0)), .depth = to_qty(iarr.at(1))});
        }
        return ladder;
    };

    return depth_update { .e = json::value_to< std::string >(o.at("e")),
                          .E = json::value_to< std::uint64_t >(o.at("E")),
                          .s = json::value_to< std::string >(o.at("s")),
                          .U = json::value_to< std::uint64_t >(o.at("U")),
                          .u = json::value_to< std::uint64_t >(o.at("u")),
                          .b = build_ladder(o.at("b").as_array()),
                          .a = build_ladder(o.at("a").as_array()) };
}

events
parse_event(const json::object &o)
{
    auto event = o.at("e").as_string();
    if (event == "agg_trade")
        return agg_trade::parse(o);
    if (event == "depthUpdate")
        return depth_update::parse(o);
    return boost::variant2::monostate();
}

}   // namespace arby::binance::protocol
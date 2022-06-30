//
// Created by ahodges on 19/05/22.
//

#ifndef ARBY_ARBY_BINANCE_PROTOCOL_EVENT_HPP
#define ARBY_ARBY_BINANCE_PROTOCOL_EVENT_HPP

#include "detail/json_conversion.hpp"
#include "trading/aggregate_book.hpp"

#include <boost/variant2.hpp>

namespace arby::binance::protocol
{

// #################### Events ####################
struct agg_trade
{
    static agg_trade
    parse(const json::object &o);

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

struct depth_update
{
    using pl_update_list = std::vector< trading::price_depth >;   // Price ladder update list

    static depth_update
    parse(const json::object &o);

    std::string    e;
    std::uint64_t  E; // Event time
    std::string    s;
    std::uint64_t  U;
    std::uint64_t  u;
    pl_update_list b;
    pl_update_list a;
};

using events = boost::variant2::variant< boost::variant2::monostate, agg_trade, depth_update >;

events
parse_event(const json::object &o);

}   // namespace arby::binance::protocol

#endif   // ARBY_ARBY_BINANCE_PROTOCOL_EVENT_HPP

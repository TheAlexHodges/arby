//
// Copyright (c) 2022 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/arby
//

#include "aggregate_book.hpp"

namespace arby::trading
{

std::ostream &
operator<<(std::ostream &os, const aggregate_book &agg_book)
{
    auto mid_opt = agg_book.mid();
    auto mid_str = mid_opt.has_value() ? to_string(*mid_opt) : "0.0";
    fmt::print(os, "MID: {}\nASKS: {}\nBIDS: {}\n", mid_str, agg_book.asks, agg_book.bids);
    return os;
}
std::optional< price_type >
aggregate_book::mid() const
{
    if (bids.empty() || asks.empty())
        return std::nullopt;
    return (bids.front().price + asks.front().price) / 2;
}
}   // namespace arby::trading

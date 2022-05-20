//
// Copyright (c) 2022 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/arby
//

#ifndef ARBY_ARBY_TRADING_AGGREGATE_BOOK_HPP
#define ARBY_ARBY_TRADING_AGGREGATE_BOOK_HPP

#include "config/format.hpp"
#include "trading/market_key.hpp"
#include "trading/types.hpp"

#include <algorithm>
#include <chrono>
#include <iosfwd>
#include <vector>

namespace arby::trading
{

struct price_depth
{
    bool
    operator<(const price_depth &rhs) const
    {
        return price < rhs.price;
    }
    bool
    operator>(const price_depth &rhs) const
    {
        return price > rhs.price;
    }

    bool
    operator==(const price_depth &rhs) const
    {
        return price == rhs.price && depth == rhs.depth;
    }

    price_type price;
    qty_type   depth;
};

namespace detail
{
struct reverse_accessor
{
    template < class T >
    static typename T::const_reverse_iterator
    front(T const &t)
    {
        return std::rbegin(t);
    }
    template < class T >
    static typename T::const_reverse_iterator
    back(T const &t)
    {
        return std::rend(t);
    }
};
}   // namespace detail

template < class Compare, class Accessor >
struct ladder_base
{
    using value_type  = price_depth;
    using ladder_type = std::vector< value_type >;

    void
    modify(value_type v)
    {
        ladder_type::iterator iter = std::lower_bound(std::begin(data_), std::end(data_), v, Compare {});
        if (iter == std::cend(data_) || iter->price != v.price)
        {
            // step with this price doesn't exist, add it.
            data_.insert(iter, std::move(v));
        }
        else
            iter->depth = std::move(v.depth);   // exists, do modify
    }

    void
    add(value_type v)
    {
        ladder_type::iterator iter = std::upper_bound(std::begin(data_), std::end(data_), v, Compare {});
        data_.insert(iter, std::move(v));
    }

    void
    remove(value_type const &v)
    {
        ladder_type::const_iterator iter = std::lower_bound(std::cbegin(data_), std::cend(data_), v, Compare {});
        if (iter == std::cend(data_) || v.price != iter->price)
            return;   // TODO throw?
        data_.erase(iter);
    }

    value_type const &
    front() const
    {
        assert(!empty());
        return *Accessor::front(data_);
    }
    value_type const &
    back() const
    {
        assert(!empty());
        return *Accessor::back(data_);
    }

    auto
    begin() const
    {
        return Accessor::front(data_);
    }
    auto
    end() const
    {
        return Accessor::back(data_);
    }

    bool
    empty() const
    {
        return data_.empty();
    }

    friend std::ostream &
    operator<<(std::ostream &os, ladder_base< Compare, Accessor > const &agg_book)
    {
        auto begin = agg_book.begin();
        auto end   = agg_book.end();
        for (; begin != end; begin++)
        {
            fmt::print(os, "[{} : {}] ", to_string(begin->price), to_string(end->depth));
        }
        return os;
    }

  protected:
    std::vector< value_type > data_;
};

// using bid_ladder = ladder_base< std::greater<> >;
// using ask_ladder = ladder_base< std::less<> >;

using bid_ladder = ladder_base< std::less<>, detail::reverse_accessor >;
using ask_ladder = ladder_base< std::greater<>, detail::reverse_accessor >;

struct aggregate_book
{
    std::optional< price_type >
    mid() const
    {
        if (bids.empty() || asks.empty())
            return std::nullopt;
        return (bids.front().price + asks.front().price) / 2;
    }

    friend std::ostream &
    operator<<(std::ostream &os, aggregate_book const &agg_book)
    {
        fmt::print(os, "MID: {}\nASKS: {}\nBIDS: {}\n", to_string(*agg_book.mid()), agg_book.asks, agg_book.bids);
        return os;
    }

    market_key     market;
    timestamp_type timestamp;
    bid_ladder     bids;
    ask_ladder     asks;
};

}   // namespace arby::trading

#endif   // ARBY_ARBY_TRADING_AGGREGATE_BOOK_HPP

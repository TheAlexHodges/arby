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
#include <map>
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

    friend std::ostream &
    operator<<(std::ostream &o, price_depth const &pd)
    {
        fmt::print(o, "[{}:{}]", to_string(pd.price), to_string(pd.depth));
        return o;
    }

    price_type price;
    qty_type   depth;
};

template < class Compare >
struct ladder_base_map
{
    void
    modify(price_depth v)
    {
        data_.insert_or_assign(v.price, std::move(v.depth));
        /*auto iter = data_.find(v.price);
        if (iter == std::end(data_)) {
            // Doesn't exist, insert

        }
         */
    }

    void
    add(price_depth v)
    {
        auto iter = data_.emplace(v.price, std::move(v.depth));
        assert(iter.second);
    }

    void
    remove(const price_depth &v)
    {
        data_.erase(v.price);
    }

    auto
    begin() const
    {
        return std::begin(data_);
    }
    auto
    end() const
    {
        return std::end(data_);
    }

    price_depth
    front() const
    {
        assert(!data_.empty());
        auto iter = std::begin(data_);
        return price_depth { iter->first, iter->second };
    }
    price_depth
    back() const
    {
        assert(!data_.empty());
        auto iter = std::end(data_);
        return price_depth { iter->first, iter->second };
    }

    bool
    empty() const
    {
        return data_.empty();
    }

    void
    clear()
    {
        data_.clear();
    }

    friend std::ostream &
    operator<<(std::ostream &os, ladder_base_map< Compare > const &ladder)
    {
        auto begin = ladder.begin();
        auto end   = ladder.end();
        for (; begin != end; begin++)
        {
            fmt::print(os, "[{} : {}] ", to_string(begin->first), to_string(begin->second));
        }
        return os;
    }

  private:
    std::map< price_type, qty_type, Compare > data_;
};

namespace detail
{
struct forward_accessor
{
    template < class T >
    static typename T::const_iterator
    begin(T const &t)
    {
        return std::begin(t);
    }
    template < class T >
    static typename T::const_iterator
    end(T const &t)
    {
        return std::end(t);
    }
};
struct reverse_accessor
{
    template < class T >
    static typename T::const_reverse_iterator
    begin(T const &t)
    {
        return std::rbegin(t);
    }
    template < class T >
    static typename T::const_reverse_iterator
    end(T const &t)
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
        assert(!v.depth.is_zero());
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
        assert(!v.depth.is_zero());
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
        return *Accessor::begin(data_);
    }
    value_type const &
    back() const
    {
        assert(!empty());
        return *Accessor::end(data_);
    }

    auto
    begin() const
    {
        return Accessor::begin(data_);
    }
    auto
    end() const
    {
        return Accessor::end(data_);
    }

    bool
    empty() const
    {
        return data_.empty();
    }

    friend std::ostream &
    operator<<(std::ostream &os, ladder_base< Compare, Accessor > const &ladder)
    {
        auto begin = ladder.begin();
        auto end   = ladder.end();
        for (; begin != end; begin++)
        {
            fmt::print(os, "[{} : {}] ", to_string(begin->price), to_string(begin->depth));
        }
        return os;
    }

  protected:
    std::vector< value_type > data_;
};

using bid_ladder = ladder_base< std::less<>, detail::reverse_accessor >;
using ask_ladder = ladder_base< std::greater<>, detail::reverse_accessor >;

using bid_ladder_forward = ladder_base< std::greater<>, detail::forward_accessor >;
using ask_ladder_forward = ladder_base< std::less<>, detail::forward_accessor >;

using bid_ladder_map = ladder_base_map< std::greater<> >;
using ask_ladder_map = ladder_base_map< std::less<> >;

struct aggregate_book
{
    std::optional< price_type >
    mid() const;

    void
    clear()
    {
        bids.clear();
        asks.clear();
    }

    friend std::ostream &
    operator<<(std::ostream &os, aggregate_book const &agg_book);

    market_key     market;
    timestamp_type timestamp;
    bid_ladder_map bids;
    ask_ladder_map asks;
};

}   // namespace arby::trading

#endif   // ARBY_ARBY_TRADING_AGGREGATE_BOOK_HPP

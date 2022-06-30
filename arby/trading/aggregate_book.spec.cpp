//
// Created by ahodges on 21/05/22.
//

#include "aggregate_book.hpp"

#include "config/format.hpp"

#include <boost/static_string.hpp>
#include <doctest/doctest.h>

using namespace arby;

TEST_SUITE_BEGIN("trading::ladder_base");
TEST_CASE("reverse_accessor")
{
    auto vec = std::vector< trading::price_depth >();

    auto get_begin = [&]() { return trading::detail::reverse_accessor::begin< decltype(vec) >(vec); };
    auto get_end   = [&]() { return trading::detail::reverse_accessor::end< decltype(vec) >(vec); };

    auto begin = get_begin();
    auto end   = get_end();
    CHECK_EQ(begin, end);

    auto elem = trading::price_depth { trading::price_type("123.345"), trading::qty_type("789.987") };
    vec.push_back(elem);
    begin            = get_begin();
    end              = get_end();
    auto deref_begin = *begin;
    auto deref_end   = *end;
    auto next        = std::next(begin);
    CHECK_NE(begin, end);
    CHECK_EQ(next, end);
    CHECK_NE(deref_begin, deref_end);
    CHECK_EQ(deref_begin, elem);
}

TEST_CASE("ladder_base<reverse_accessor>::begin/end")
{
    // begin/end
    auto ladder = trading::ask_ladder();
    auto begin  = ladder.begin();
    auto end    = ladder.end();
    CHECK_EQ(begin, end);
    fmt::print("empty: {}\n", ladder);

    auto elem = trading::price_depth { trading::price_type("123.345"), trading::qty_type("789.987") };
    fmt::print("elem: {}\n", elem);
    ladder.modify(elem);
    begin = ladder.begin();
    end   = ladder.end();
    CHECK_NE(begin, end);
    auto next = std::next(begin);
    CHECK_EQ(next, end);
    auto deref_begin = *begin;
    auto deref_end   = *end;
    CHECK_NE(deref_begin, deref_end);
    CHECK_EQ(elem, deref_begin);
    fmt::print("one elem: {}\n", ladder);
}

template < class Ladder >
void
fill_ladder(Ladder &ladder, std::size_t count)
{
    for (std::uint64_t x = 0; x < count; ++x)
    {
        ladder.add(trading::price_depth { trading::price_type(std::to_string(x)), trading::qty_type(std::to_string(x)) });
    }
}
template void
fill_ladder< trading::ladder_base_map< std::less<> > >(trading::ladder_base_map< std::less<> > &, std::size_t);
template void
fill_ladder< trading::ladder_base< std::less<>, trading::detail::forward_accessor > >(
    trading::ladder_base< std::less<>, trading::detail::forward_accessor > &,
    std::size_t);
template void
fill_ladder< trading::ladder_base< std::less<>, trading::detail::reverse_accessor > >(
    trading::ladder_base< std::less<>, trading::detail::reverse_accessor > &,
    std::size_t);

TEST_CASE("ladder_base_map traversal benchmark 5000 depths")
{
    // Build ladder
    auto ladder = trading::ladder_base_map< std::less<> >();
    fill_ladder(ladder, 5000);

    // Iterate
    auto count     = 100000;
    auto qty_total = trading::qty_type(0);
    auto start     = std::chrono::steady_clock::now();
    for (auto x = 0; x < count; ++x)
    {
        auto begin = ladder.begin();
        auto end   = ladder.end();
        for (; begin != end; ++begin)
        {
            qty_total += begin->second;
        }
    }
    auto diff = std::chrono::steady_clock::now() - start;
    DOCTEST_MESSAGE(fmt::format("[{}] {} x iterations took total: {} ({} per iteration)", qty_total, count, diff, diff / count));
}

TEST_CASE("ladder_base <forward> benchmark 5000 depths")
{
    // Build ladder
    auto ladder = trading::ask_ladder_forward();
    fill_ladder(ladder, 5000);

    // Iterate
    auto count     = 100000;
    auto qty_total = trading::qty_type(0);
    auto start     = std::chrono::steady_clock::now();
    for (auto x = 0; x < count; ++x)
    {
        auto begin = ladder.begin();
        auto end   = ladder.end();
        for (; begin != end; ++begin)
        {
            qty_total += begin->depth;
        }
    }
    auto diff = std::chrono::steady_clock::now() - start;
    DOCTEST_MESSAGE(fmt::format("[{}] {} x iterations took total: {} ({} per iteration)", qty_total, count, diff, diff / count));
}

TEST_CASE("ladder_base <forward> benchmark 5000 depths")
{
    // Build ladder
    auto ladder = trading::ask_ladder();
    fill_ladder(ladder, 5000);

    // Iterate
    auto count     = 100000;
    auto qty_total = trading::qty_type(0);
    auto start     = std::chrono::steady_clock::now();
    for (auto x = 0; x < count; ++x)
    {
        auto begin = ladder.begin();
        auto end   = ladder.end();
        for (; begin != end; ++begin)
        {
            qty_total += begin->depth;
        }
    }
    auto diff = std::chrono::steady_clock::now() - start;
    DOCTEST_MESSAGE(fmt::format("[{}] {} x iterations took total: {} ({} per iteration)", qty_total, count, diff, diff / count));
}

TEST_SUITE_END();

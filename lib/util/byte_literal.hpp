//
// Created by ahodges on 24/05/22.
//

#ifndef ARBY_LIB_UTIL_BYTE_LITERAL_HPP
#define ARBY_LIB_UTIL_BYTE_LITERAL_HPP

#include <cstddef>

namespace arby::byte_literals
{

namespace detail
{
// https://stackoverflow.com/a/1505791/19156491
template < unsigned long long exp >
auto constexpr pow(unsigned long long const x)
{
    if constexpr (exp == 0)
        return 1;
    if constexpr (exp == 1)
        return x;

    auto tmp = pow< exp / 2 >(x);
    if constexpr ((exp % 2) == 0)
        return tmp * tmp;
    else
        return x * tmp * tmp;
}
}   // namespace detail
inline constexpr auto operator""_Bit(unsigned long long const x)
{
    return x;
}

inline constexpr auto operator""_KB(unsigned long long const x)
{
    return x * 1000;
}
inline constexpr auto operator""_KiB(unsigned long long const x)
{
    return x * 1024;
}
inline constexpr auto operator""_MB(unsigned long long const x)
{
    return x * detail::pow< 2 >(1_KB);
}
inline constexpr auto operator""_MiB(unsigned long long const x)
{
    return x * detail::pow< 2 >(1_KiB);
}
inline constexpr auto operator""_GB(unsigned long long const x)
{
    return x * detail::pow< 3 >(1_MB);
}
inline constexpr auto operator""_GiB(unsigned long long const x)
{
    return x * detail::pow< 3 >(1_MiB);
}
}   // namespace arby::byte_literals

#endif   // ARBY_LIB_UTIL_BYTE_LITERAL_HPP

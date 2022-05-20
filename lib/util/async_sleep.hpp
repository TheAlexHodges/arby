//
// Created by ahodges on 18/05/22.
//

#ifndef ARBY_LIB_UTIL_ASYNC_SLEEP_HPP
#define ARBY_LIB_UTIL_ASYNC_SLEEP_HPP

#include "config/asio.hpp"

namespace arby::util
{

template < class Timer = asio::steady_timer, class Duration = typename Timer::duration >
asio::awaitable< void >
async_sleep(const Duration &duration)
{
    auto this_exec = co_await asio::this_coro::executor;
    auto timer     = Timer(this_exec);
    timer.expires_after(duration);
    co_await timer.async_wait(asio::use_awaitable);
}
}   // namespace arby::util
#endif   // ARBY_LIB_UTIL_ASYNC_SLEEP_HPP

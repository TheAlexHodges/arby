//
// Copyright (c) 2022 Richard Hodges (hodges.r@gmail.com)
// Copyright (c) 2022 Alexander Hodges (alexander.hodges.personal@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/arby
//

#ifndef ARBY_LIB_ASIOEX_EXECUTE_ON_HPP
#define ARBY_LIB_ASIOEX_EXECUTE_ON_HPP

#include "config/asio.hpp"

namespace arby {
    namespace asioex {
        template<class Work>
        auto
        execute_on(asio::any_io_executor const &exec, Work &&w) -> decltype(w()) {
            using asio::co_spawn;
            using asio::use_awaitable;

            auto this_exec = co_await asio::this_coro::executor;
            if (exec == this_exec) {
                co_return co_await w();
            } else {
                co_return co_await co_spawn(exec, std::forward<Work>(w), use_awaitable);
            }
        }
    }   // namespace asioex
}   // namespace arby

#endif   // ARBY_LIB_ASIOEX_EXECUTE_ON_HPP


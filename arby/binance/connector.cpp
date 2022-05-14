//
// Copyright (c) 2022 Richard Hodges (hodges.r@gmail.com)
// Copyright (c) 2022 Alexander Hodges (alexander.hodges.personal@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/arby
//

#include "binance/connector.hpp"
#include "entity/entity_base.hpp"

namespace arby
{
namespace binance
{
asio::awaitable< util::cross_executor_connection >
connector::watch_connection_state(impl_class::connection_state_slot slot)
{
    auto this_exec = co_await asio::this_coro::executor;
    auto my_exec = get_executor();

    connection_state current;
    util::
}
}   // namespace binance
}   // namespace arby

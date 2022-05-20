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

#include "asioex/execute_on.hpp"
#include "entity/entity_base.hpp"

namespace arby
{
namespace binance
{
asio::awaitable< util::cross_executor_connection >
connector::watch_messages(impl_class::message_slot slot)
{
    co_return co_await asioex::execute_on(
        get_executor(),
        [&]() -> asio::awaitable< util::cross_executor_connection >
        {
            auto impl = get_implementation();
            co_return util::cross_executor_connection { impl, impl->watch_messages(std::move(slot)) };
        });
}

asio::awaitable< std::tuple< util::cross_executor_connection, connection_state > >
connector::watch_connection_state(connection_state_slot slot)
{
    connection_state current;

    co_return co_await asioex::execute_on(
        get_executor(),
        [&]() -> asio::awaitable< std::tuple< util::cross_executor_connection, connection_state > >
        {
            auto impl = get_implementation();
            auto conn = impl->watch_connection_state(current, std::move(slot));
            co_return std::make_tuple(util::cross_executor_connection(impl, conn), current);
        });
}
}   // namespace binance
}   // namespace arby

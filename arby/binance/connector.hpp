//
// Copyright (c) 2022 Richard Hodges (hodges.r@gmail.com)
// Copyright (c) 2022 Alexander Hodges (alexander.hodges.personal@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/arby
//

#ifndef ARBY_ARBY_BINANCE_CONNECTOR_HPP
#define ARBY_ARBY_BINANCE_CONNECTOR_HPP

#include "config/asio.hpp"
#include "detail/connector_impl.hpp"
#include "entity/entity_base.hpp"

#include <memory>

namespace arby
{
namespace binance
{
struct connector : entity::entity_handle< detail::connector_impl >
{
    using impl_class = detail::connector_impl;
    using impl_type  = std::shared_ptr< impl_class >;

    connector(impl_type impl)
    : entity::entity_handle< impl_class >(impl)
    {
    }

    connector(asio::any_io_executor exec, asio::ssl::context &sslctx, detail::binance_connector_args args)
    : entity::entity_handle<impl_class>(exec, sslctx, args)
    {
    }
};
}   // namespace binance
}   // namespace arby

#endif   // ARBY_ARBY_BINANCE_CONNECTOR_HPP

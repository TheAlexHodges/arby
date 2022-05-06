//
// Copyright (c) 2022 Richard Hodges (hodges.r@gmail.com)
// Copyright (c) 2022 Alexander Hodges (alexander.hodges.personal@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/arby
//

#include "connector_impl.hpp"

#include "asioex/scoped_interrupt.hpp"
#include "util/cross_executor_connection.hpp"

#include <fmt/format.h>

namespace arby
{
namespace binance
{
namespace detail
{

void
merge(entity::entity_key &target, binance_connector_args const &args)  {
    target.set("binance.Host", args.host);
    target.set("binance.Port", args.port);
    target.set("binance.Path", args.path);
}


connector_impl::connector_impl(asio::any_io_executor exec, ssl::context &ioc, binance_connector_args args)
: entity::entity_base(std::move(exec))
, ssl_ctx_(ioc)
, args_(std::move(args))
{
}
void
connector_impl::extend_summary(std::string &buffer) const
{
}

void
connector_impl::handle_start()
{

}
void
connector_impl::handle_stop()
{
}

}   // namespace detail
}   // namespace binance
}   // namespace arby
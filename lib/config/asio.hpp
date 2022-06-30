#ifndef ARBY_CONFIG_ASIO_HPP
#define ARBY_CONFIG_ASIO_HPP

#include "error.hpp"

#include <boost/asio.hpp>
#include <boost/asio/experimental/append.hpp>
#include <boost/asio/experimental/as_tuple.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/experimental/deferred.hpp>
#include <boost/asio/experimental/prepend.hpp>
#include <boost/asio/experimental/promise.hpp>
#include <boost/asio/ssl.hpp>

namespace arby
{
namespace asio = boost::asio;
namespace ip   = asio::ip;
namespace ssl  = asio::ssl;
using tcp      = ip::tcp;
}   // namespace arby

#endif

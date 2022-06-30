//
// Created by ahodges on 26/05/22.
//

#ifndef ARBY_LIB_CONFIG_ERROR_HPP
#define ARBY_LIB_CONFIG_ERROR_HPP

#include <boost/system/system_error.hpp>

namespace arby
{
namespace sys = boost::system;
using sys::error_code;
using sys::system_error;
}   // namespace arby

#endif   // ARBY_LIB_CONFIG_ERROR_HPP

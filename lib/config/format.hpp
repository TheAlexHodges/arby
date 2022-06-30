//
// Created by ahodges on 18/05/22.
//

#ifndef ARBY_LIB_CONFIG_FORMAT_HPP
#define ARBY_LIB_CONFIG_FORMAT_HPP

#include <fmt/format.h>
#include <fmt/ostream.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <fmt/chrono.h>
#pragma clang diagnostic pop
#else
#include <fmt/chrono.h>
#endif

#include <spdlog/spdlog.h>

namespace arby
{
}

#endif   // ARBY_LIB_CONFIG_FORMAT_HPP

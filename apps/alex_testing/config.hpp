//
// Created by ahodges on 24/05/22.
//

#ifndef ARBY_APPS_ALEX_TESTING_CONFIG_HPP
#define ARBY_APPS_ALEX_TESTING_CONFIG_HPP

#include "config/error.hpp"
#include "config/filesystem.hpp"
#include "config/json.hpp"
#include "util/byte_literal.hpp"

#define MAX_CONFIG_FILE_SIZE 1_MB


//template<typename T>
//concept

namespace app
{
using namespace arby::byte_literals;
struct config
{
    // Traits
    using buffer = std::vector< char >;

    static config
    load(arby::fs::path const &path);

  private:
    config(arby::json::value &&v, buffer &&b)
    : json_cfg_(v)
    , buf_(b)
    {
    }

    arby::json::value json_cfg_;
    buffer            buf_;
};
}   // namespace

#endif   // ARBY_APPS_ALEX_TESTING_CONFIG_HPP

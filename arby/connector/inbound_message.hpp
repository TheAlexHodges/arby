//
// Copyright (c) 2022 Richard Hodges (hodges.r@gmail.com)
// Copyright (c) 2022 Alexander Hodges (alexander.hodges.personal@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/arby
//

#ifndef ARBY_ARBY_CONNECTOR_INBOUND_MESSAGE_HPP
#define ARBY_ARBY_CONNECTOR_INBOUND_MESSAGE_HPP

#include "config/json.hpp"
#include "trading/types.hpp"

namespace arby::connector
{

template<class Buffer>
class inbound_message
{
    trading::timestamp_type timestamp_;
    Buffer      buffer_;
    json::value             value_;
    json::string            type_;
    json::object const     *object_;

  public:
    inbound_message(std::size_t capacity)
    : buffer_(capacity)
    {
    }

    json::object const &
    object() const
    {
        return *object_;
    }

    Buffer &
    prepare()
    {
        timestamp_ = std::chrono::system_clock::now();
        object_    = nullptr;
        type_.clear();
        value_ = nullptr;
        buffer_.clear();
        return buffer_;
    }

    std::string_view
    view() const
    {
        auto d = buffer_.data();
        return std::string_view(static_cast< const char * >(d.data()), d.size());
    }

    json::string const &
    type() const
    {
        return type_;
    }

    trading::timestamp_type
    timestamp() const
    {
        return timestamp_;
    }

    void
    commit()
    {
        timestamp_ = std::chrono::system_clock::now();
        auto data_view     = view();
        //auto v1    = json::string_view(v.begin(), v.end());
        value_     = json::parse(data_view);
        if (auto outer = value_.if_object(); outer && !outer->empty())
        {
            auto &[k, v] = *outer->begin(); // Key, Value
            type_.assign(k.begin(), k.end());
            object_ = v.if_object();
        }
    }
};
}   // namespace arby::connector

#endif   // ARBY_ARBY_CONNECTOR_INBOUND_MESSAGE_HPP

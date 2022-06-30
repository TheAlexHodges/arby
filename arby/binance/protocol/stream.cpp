//
// Created by ahodges on 19/05/22.
//

#include "stream.hpp"

namespace arby::binance::protocol
{
stream_response
stream_response::parse(const json::object &o)
{
    using namespace detail::json_conversion;
    return stream_response { .result = to_opt_string(o.at("result")), .id = json::value_to< std::uint64_t >(o.at("id")) };
}

stream_event
stream_event::parse(const json::object &o)
{
    return stream_event { .stream = json::value_to< std::string >(o.at("stream")), .data = parse_event(o.at("data").as_object()) };
}

stream_message
parse_stream(const json::value &v)
{
    using namespace detail::json_conversion;

    auto &o = v.as_object();
    for (auto begin = o.begin(); begin != o.end(); begin++)
    {
        auto key = begin->key();
        [[likely]] if (key == "stream" || key == "data")   // Fast path, most messages are stream_event messages
            return stream_event::parse(o);
        if (key == "result")
            return stream_response::parse(o);
        else
            throw std::runtime_error("invalid stream message");
    }
    throw std::runtime_error("invalid empty message");
}
}   // namespace arby::binance::protocol
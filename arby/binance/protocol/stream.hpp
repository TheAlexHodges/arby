//
// Created by ahodges on 19/05/22.
//

#ifndef ARBY_ARBY_BINANCE_PROTOCOL_STREAM_HPP
#define ARBY_ARBY_BINANCE_PROTOCOL_STREAM_HPP

#include "event.hpp"

namespace arby::binance::protocol
{
// #################### Client ####################
template < class... String >
std::string
subscribe(unsigned int id, const String &&...params)
{
    auto val = json::value({ { "method", "SUBSCRIBE" }, { "params", json::array({ params... }) }, { "id", id } });
    return serialize(val);
}

// #################### Server ####################

struct stream_response
{
    static stream_response
                                 parse(const json::object &o);
    std::optional< std::string > result;
    std::uint64_t                id;
};

struct stream_event
{
    static stream_event
    parse(const json::object &o);

    std::string stream;
    events      data;
};

typedef boost::variant2::variant< stream_response, stream_event > stream_message;

stream_message
parse_stream(const json::value &v);

}   // namespace arby::binance::protocol

#endif   // ARBY_ARBY_BINANCE_PROTOCOL_STREAM_HPP

//
// Created by ahodges on 08/05/22.
//

#ifndef ARBY_ARBY_CONNECTOR_CONNECTOR_TRAITS_HPP
#define ARBY_ARBY_CONNECTOR_CONNECTOR_TRAITS_HPP

#include "config/websocket.hpp"
#include "connection_state.hpp"
#include "inbound_message.hpp"

#include <boost/signals2.hpp>

namespace arby::connector
{
struct connector_traits
{
    using executor_type = asio::any_io_executor;
    using tcp_layer     = tcp::socket;
    using tls_layer     = asio::ssl::stream< tcp_layer >;
    using ws_stream     = websocket::stream< tls_layer >;

    using inbound_message_type = inbound_message< beast::flat_buffer >;
    using connection_id        = std::uint64_t;

    // Note that the signal type is not thread-safe. You must only interact with
    // the signals while on the same executor and thread as the connector
    using message_signal =
        boost::signals2::signal_type< void(connection_id, std::shared_ptr< inbound_message_type const >),
                                      boost::signals2::keywords::mutex_type< boost::signals2::dummy_mutex > >::type;

    using message_slot          = message_signal::slot_type;
    using message_extended_slot = message_signal::extended_slot_type;

    using connection_state_signal =
        boost::signals2::signal_type< void(connection_id, connection_state),
                                      boost::signals2::keywords::mutex_type< boost::signals2::dummy_mutex > >::type;
    using connection_state_slot          = connection_state_signal::slot_type;
    using connection_state_extended_slot = connection_state_signal::extended_slot_type;
};
}   // namespace arby::connector

#endif   // ARBY_ARBY_CONNECTOR_CONNECTOR_TRAITS_HPP

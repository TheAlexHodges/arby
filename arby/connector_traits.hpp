//
// Created by ahodges on 08/05/22.
//

#ifndef ARBY_ARBY_CONNECTOR_TRAITS_HPP
#define ARBY_ARBY_CONNECTOR_TRAITS_HPP

#include "config/websocket.hpp"
#include "connection_state.hpp"
#include <boost/signals2.hpp>

namespace arby {
struct connector_traits {
    using tcp_layer                   = tcp::socket;
    using tls_layer                   = asio::ssl::stream< tcp_layer >;
    using ws_stream                   = websocket::stream< tls_layer >;



    using connection_state_signal =
        boost::signals2::signal_type< void(connection_state),
                                      boost::signals2::keywords::mutex_type< boost::signals2::dummy_mutex > >::type;
    using connection_state_slot          = connection_state_signal::slot_type;
    using connection_state_extended_slot = connection_state_signal::extended_slot_type;

};
}

#endif   // ARBY_ARBY_CONNECTOR_TRAITS_HPP

#pragma once

#include <bencode/bencode.hpp>
#include "dottorrent/dht_node.hpp"

BENCODE_SERIALIZES_TO_LIST(dottorrent::dht_node)

namespace dottorrent {

template <bencode::event_consumer EventConsumer>
constexpr void bencode_connect(
        bencode::customization_point_type<dottorrent::dht_node>,
        EventConsumer& consumer,
        const dht_node& value)
{
    consumer.begin_list();
    consumer.string(value.url);
    consumer.integer(value.port);
    consumer.end_list();
}

} // namespace dottorrent
#pragma once

#include <bencode/bencode.hpp>

#include "dottorrent/announce_url_list.hpp"

BENCODE_SERIALIZES_TO_LIST(dottorrent::announce_url_list)

namespace dottorrent {

template <bencode::event_consumer EC>
constexpr void bencode_connect(
        bencode::customization_point_type<dottorrent::announce_url_list>,
        EC& consumer,
        const dottorrent::announce_url_list& value)
{
    consumer.begin_list();
    if (!value.empty()) {
        consumer.begin_list();
        int current_tier = 0;
        for (const auto& [url, tier] : value) {
            if (current_tier != tier) {
                consumer.end_list();
                consumer.begin_list();
            }
            consumer.string(url);
            consumer.list_item();
        }
        consumer.end_list();
    }
    consumer.end_list();
}

} // namespace dottorrent
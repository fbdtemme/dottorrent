#pragma once
#include <compare>
#include <cstdint>
#include <string>

namespace dottorrent {

struct dht_node
{
    std::string url;
    std::uint16_t port;

    dht_node() noexcept = default;
    dht_node(std::string_view url, std::uint16_t port)
        : url(url)
        , port(port)
    {}

    dht_node(const dht_node&) = default;
    dht_node(dht_node&&) = default;
    dht_node& operator=(const dht_node&) = default;
    dht_node& operator=(dht_node&&) = default;

    bool operator==(const dht_node& that) const = default;
    std::strong_ordering operator<=>(const dht_node& that) const  = default;
};

} // namespace dottorrent

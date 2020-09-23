#pragma once

#include <algorithm>
#include <compare>
#include <iterator>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>


#include <gsl/gsl>
#include <bencode/bencode.hpp>


namespace dottorrent {

struct announce_url {
    explicit announce_url(std::string_view url, std::size_t tier = 0)
            : url(url)
            , tier(tier)
    {}

    announce_url(const announce_url&) = default;
    announce_url(announce_url&&) = default;
    announce_url& operator=(const announce_url&) = default;
    announce_url& operator=(announce_url&&) = default;

    operator std::string_view() const               // NOLINT
    { return url; }

    bool operator==(const announce_url& that) const = default;

    std::weak_ordering operator<=>(const announce_url& that) const
    {
        if (auto order = std::compare_weak_order_fallback(url, that.url);
                 order != std::weak_ordering::equivalent ){
            return order;
        }
        return tier <=> that.tier;
    };

    // friends for structured binding support
    template <std::size_t N> friend const auto& get(const announce_url&);
    template <std::size_t N> friend auto& get(announce_url&);

    std::string url;
    std::size_t tier;
};
}


//-----------------------------------------------------------------------------//
// Structured bindings support                                                 //
//-----------------------------------------------------------------------------//

namespace dottorrent {
template <std::size_t N>
const auto& get(const announce_url& a) {
    if      constexpr (N == 0) return a.url;
    else if constexpr (N == 1) return a.tier;
}

template <std::size_t N>
auto& get(announce_url& a) {
    if      constexpr (N == 0) return a.url;
    else if constexpr (N == 1) return a.tier;
}
}

namespace std {
template<>
struct tuple_size<dottorrent::announce_url>
        : std::integral_constant<std::size_t, 2> {};

template<std::size_t N>
struct tuple_element<N, dottorrent::announce_url> {
    using type = decltype(get<N>(std::declval<dottorrent::announce_url>()));
};
}

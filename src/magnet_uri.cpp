#include "dottorrent/magnet_uri.hpp"

namespace dottorrent {

namespace detail {

std::vector<std::string_view> split_query_string(std::string_view uri_query_string)
{
    std::vector<std::string_view> parts;

    auto it = uri_query_string.begin();
    auto end = uri_query_string.end();

    while (it != end) {
        auto ampersand_it = rng::find(it, end, '&');
        if (ampersand_it != end) {
            parts.emplace_back(it, ampersand_it);
            it = std::next(ampersand_it);
        }
        else {
            parts.emplace_back(it, ampersand_it);
            break;
        }
    }
    return parts;
}

std::pair<std::string_view, std::string_view>
split_key_value_pair(std::string_view uri_query_string)
{
    auto it = rng::find(uri_query_string, '=');
    if (it == uri_query_string.end()) {
        throw std::invalid_argument("not a key-value pair");
    }
    auto offset = std::distance(uri_query_string.begin(), it);
    return {uri_query_string.substr(0, offset), uri_query_string.substr(offset+1)};
}

std::vector<std::string_view> split_keywords_string(std::string_view keywords)
{
    std::vector<std::string_view> parts;

    auto it = keywords.begin();
    auto end = keywords.end();

    while (it != end) {
        auto plus_it = rng::find(it, end, '+');
        if (plus_it != end) {
            parts.emplace_back(it, plus_it);
            it = std::next(plus_it);
        }
        else {
            parts.emplace_back(it, plus_it);
            break;
        }
    }
    return parts;
}


}

magnet_uri make_magnet_uri(std::string_view uri)
{
    std::string_view magnet_uri_introducer = "magnet:?";
    magnet_uri result;

    std::size_t pos = 0;

    if (uri.substr(0, magnet_uri_introducer.size()) != magnet_uri_introducer) {
        throw std::invalid_argument("not a magnet uri");
    }

    auto query_string = uri.substr(magnet_uri_introducer.size());
    auto query_parts = detail::split_query_string(query_string);

    for (auto part: query_parts) {
        auto [key, value] = detail::split_key_value_pair(part);

        if (key == "dn") {
            result.set_display_name(detail::percent_decode(value));
        }
        else if (key == "xl") {
            std::size_t xl_value;
            auto [ptr, ec] = std::from_chars(value.begin(), value.end(), xl_value);
            if (ptr != value.end()) {
                throw std::invalid_argument("magnet xl field is not an integral value");
            }
            if (ec != std::errc{}) {
                throw std::system_error(std::make_error_code(ec));
            }
            result.set_size(xl_value);
        }
        if (key == "xt") {
            result.add_topic(detail::percent_decode(value));
        }
        else if (key == "tr") {
            result.add_tracker(detail::percent_decode(value));
        }
        else if (key == "ws") {
            result.add_web_seed(detail::percent_decode(value));
        }
        else if (key == "as") {
            result.add_acceptable_source(detail::percent_decode(value));
        }
        else if (key == "xs") {
            result.set_exact_source(detail::percent_decode(value));
        }
        else if (key == "kt") {
            auto keywords_string = detail::percent_decode(value);
            auto keywords = detail::split_keywords_string(keywords_string);
            for (auto kw : keywords) {
                result.add_keyword(kw);
            }
        }
        else if (key == "mt") {
            result.set_manifest(detail::percent_decode(value));
        }
        else if (key == "x.pe") {
            result.add_peer_address(detail::percent_decode(value));
        }
    }
    return result;
}

magnet_uri make_magnet_uri(const metafile& m, protocol protocol)
{
    magnet_uri magnet{};

    auto format_topic_v1 = [](const metafile& m){
        return fmt::format("urn:btih:{}", info_hash_v1(m).hex_string());
    };

    auto format_topic_v2 = [](const metafile& m){
        return fmt::format("urn:btmh:1220{}", info_hash_v2(m).hex_string());
    };

    if (m.storage().protocol() == protocol::hybrid) {
        if ((protocol & protocol::v1) == protocol::v1)  {
            magnet.add_topic(format_topic_v1(m));
        }
        if ((protocol & protocol::v2) == protocol::v2) {
            magnet.add_topic(format_topic_v2(m));
        }
    }
    else if (m.storage().protocol() == protocol::v1) {
        magnet.add_topic(format_topic_v1(m));
    }
    else if (m.storage().protocol() == protocol::v2) {
        magnet.add_topic(format_topic_v2(m));
    }
    else {
        throw std::invalid_argument("cannot create magnet link for unfinished metafile");
    }
    magnet.set_display_name(m.name());
    for (const auto& tr : m.trackers()) {
        magnet.add_tracker(tr.url);
    }
    for (const auto& ws : m.web_seeds()) {
        magnet.add_web_seed(ws);
    }
    for (const auto& dn : m.dht_nodes()) {
        magnet.add_peer_address(dn.url, dn.port);
    }
    return magnet;
}

std::string encode_magnet_uri(const magnet_uri& magnet)
{
    std::string result {};
    encode_magnet_uri(magnet, std::back_inserter(result));
    return result;
}
}
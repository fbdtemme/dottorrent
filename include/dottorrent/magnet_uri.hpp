#pragma once

#include <string>
#include <vector>
#include <ranges>
#include <charconv>
#include <unordered_set>
#include <dottorrent/metafile.hpp>

#include "dottorrent/percent_encoding.hpp"


//tr	address TRacker	Tracker URL; used to obtain resources for BitTorrent downloads without a need for DHT support.[2] The value must be URL encoded.
//tr=http%3A%2F%2Fexample.org%2Fannounce

namespace dottorrent {

namespace rng = std::ranges;

struct magnet_uri
{
    magnet_uri() = default;

    std::vector<std::string> topics() const noexcept
    {
        return topics_;
    }

    const std::string& display_name() const noexcept
    {
        return display_name_;
    }

    std::size_t size() const noexcept
    {
        return size_;
    }

    const std::vector<std::string>& web_seeds() const noexcept
    {
        return web_seeds_;
    }

    const std::vector<std::string>& trackers() const noexcept
    {
        return trackers_;
    }

    const std::vector<std::string>& acceptable_sources() const noexcept
    {
        return acceptable_sources_;
    }

    const std::string& exact_source() const noexcept
    {
        return exact_source_;
    }

    const std::unordered_set<std::string>& keywords() const noexcept
    {
        return keywords_;
    }

    const std::vector<std::string>& peer_adresses() const noexcept
    {
        return peer_adresses_;
    }

    const std::string& manifest() const noexcept
    {
        return manifest_;
    }

    void add_topic(const std::string& topic)
    {
        topics_.emplace_back(topic);
    }

    void add_topic(std::string&& topic)
    {
        topics_.emplace_back(std::move(topic));
    }

    void add_web_seed(std::string_view seed)
    {
        web_seeds_.emplace_back(std::string(seed));
    }

    void add_web_seed(std::string&& seed)
    {
        web_seeds_.emplace_back(std::move(seed));
    }

    void add_tracker(std::string_view tracker)
    {
        trackers_.emplace_back(std::string(tracker));
    }

    void add_tracker(std::string&& tracker)
    {
        trackers_.emplace_back(std::move(tracker));
    }

    void add_acceptable_source(std::string_view acceptable_source)
    {
        acceptable_sources_.emplace_back(std::string(acceptable_source));
    }

    void add_acceptable_source(std::string&& acceptable_source)
    {
        acceptable_sources_.emplace_back(std::move(acceptable_source));
    }

    void add_keyword(std::string_view keyword)
    {
        keywords_.emplace(std::string(keyword));
    }

    void add_keyword(std::string&& keyword)
    {
        keywords_.emplace(keyword);
    }

    void set_size(std::size_t size)
    {
        size_ = size;
    }

    void set_display_name(std::string_view name)
    {
        display_name_ = name;
    }

    void set_display_name(std::string&& name)
    {
        display_name_ = std::move(name);
    }



    void set_manifest(std::string_view manifest)
    {
        manifest_ = std::string(manifest);
    }

    void set_manifest(std::string&& manifest)
    {
        manifest_ = std::move(manifest);
    }

    void set_exact_source(const std::string& source)
    {
        exact_source_ = source;
    }

    void set_exact_source(std::string&& source)
    {
        exact_source_ = std::move(source);
    }

    void add_peer_adress(const std::string& peer_adress)
    {
        peer_adresses_.emplace_back(peer_adress);
    }
    
    void add_peer_address(std::string&& peer_adress)
    {
        peer_adresses_.emplace_back(std::move(peer_adress));
    }
    
    void add_peer_address(std::string_view address, std::uint16_t port)
    {
        peer_adresses_.emplace_back(fmt::format("{}:{}", address, port));
    }

private:
    //dn	Display Name	A filename to display to the user, for convenience.
    std::string display_name_;
    //xl	eXact Length	Size (in bytes)
    std::size_t size_;
    //xt	eXact Topic	URN containing file hash.
    // This is the most crucial part of the magnet link, and is used to find and verify the specified file.
    // The URN is specific to the protocol, so a file hash URN under btih (BitTorrent) would be completely different than the file hash URN for ed2k
    std::vector<std::string> topics_;
//    tr	address TRacker	Tracker URL; used to obtain resources for BitTorrent downloads without a need for DHT support.[2] The value must be URL encoded.
//    tr=http%3A%2F%2Fexample.org%2Fannounce
    std::vector<std::string> trackers_;
    //ws	Web Seed	The payload data served over HTTP(S)
    std::vector<std::string> web_seeds_;
    //as	Acceptable Source	Refers to a direct download from a web server. Regarded as only a fall-back source in case a client is unable to locate and/or download the linked-to file in its supported P2P network(s)
    //as=[web link to file(URL encoded)]
    std::vector<std::string> acceptable_sources_;
    //xs	eXact Source	Either an HTTP (or HTTPS, FTP, FTPS, etc.) download source for the file pointed to by the Magnet link,
    // the address of a P2P source for the file or the address of a hub (in the case of DC++),
    // by which a client tries to connect directly, asking for the file and/or its sources.
    // This field is commonly used by P2P clients to store the source, and may include the file hash.
    //xs=http://[Client Address]:[Port of client]/uri-res/N2R?[ URN containing a file hash ]
    //xs=http://192.0.2.27:6346/uri-res/N2R?urn:sha1:FINYVGHENTHSMNDSQQYDNLPONVBZTICF
    std::string exact_source_;
    //kt	Keyword Topic	Specifies a string of search keywords to search for in P2P networks, rather than a particular file
    //kt=martin+luther+king+mp3
    std::unordered_set<std::string> keywords_;

    /// x.pe A peer address expressed as hostname:port, ipv4-literal:port or [ipv6-literal]:port.
    /// This parameter can be included to initiate a direct metadata transfer between two clients while reducing the need for external peer sources.
    /// It should only be included if the client can discover its public IP address and determine its reachability.
    /// Note: Since no URI scheme identifier has been allocated for bittorrent xs= is not used for this purpose.
    std::vector<std::string> peer_adresses_;
    //mt	Manifest Topic	Link to the metafile that contains a list of magneto (MAGMA – MAGnet MAnifest); i.e. a link to a list of links
    //mt=http://example.org/all-my-favorites.rss
    //mt=urn:sha1:3I42H3S6NNFQ2MSVX7XZKYAYSCX5QBYJ
    std::string manifest_;
};


template <std::output_iterator<char> OutputIterator>
void encode_magnet_uri(const magnet_uri& magnet, OutputIterator out)
{
    using namespace std::string_view_literals;

    std::string uri = "magnet:?";
    rng::copy(uri, out);

    bool first = true;
    // topics
    for (const auto& topic : magnet.topics()) {
        if (first) {
            rng::copy("xt="sv, out);
            first = false;
        }
        else {
            rng::copy("&xt="sv, out);
        }
        rng::copy(topic, out);
    }

    // display name
    if (!magnet.display_name().empty()) {
        rng::copy("&dn="sv, out);
        rng::copy(detail::percent_encode(magnet.display_name()), out);
    }

    if (magnet.size() != 0) {
        // exact length
        rng::copy("&xl="sv, out);
        rng::copy(detail::percent_encode(std::to_string(magnet.size())), out);
    }
    // trackers
    for (const auto& tr : magnet.trackers()) {
        rng::copy("&tr="sv, out);
        rng::copy(detail::percent_encode(tr), out);
    }

    // webseeds
    for (const auto& ws : magnet.web_seeds()) {
        rng::copy("&ws="sv, out);
        rng::copy(detail::percent_encode(ws), out);
    }

    // Acceptable sources
    for (const auto& as : magnet.acceptable_sources()) {
        rng::copy("&as="sv, out);
        rng::copy(detail::percent_encode(as), out);
    }


    // Exact source
    if (!magnet.exact_source().empty()) {
        rng::copy("&xs="sv, out);
        rng::copy(detail::percent_encode(magnet.exact_source()), out);
    }

    // keywords
    if (!magnet.keywords().empty()) {
        auto it = magnet.keywords().begin();
        rng::copy(detail::percent_encode(*it++), out);

        while (it != magnet.keywords().end()) {
            *out++ = '+';
            rng::copy(detail::percent_encode(*it), out);
        }
    }

    // manifest
    if (!magnet.manifest().empty()) {
        rng::copy("&mt="sv, out);
        rng::copy(detail::percent_encode(magnet.manifest()), out);
    }

    // Peers
    for (const auto& pe : magnet.peer_adresses()) {
        rng::copy("&x.pe="sv, out);
        rng::copy(detail::percent_encode(pe), out);
    }
}


std::string encode_magnet_uri(const magnet_uri& magnet);


namespace detail {

std::vector<std::string_view> split_query_string(std::string_view uri_query_string);

std::pair<std::string_view, std::string_view>
split_key_value_pair(std::string_view uri_query_string);

std::vector<std::string_view> split_keywords_string(std::string_view keywords);

}

/// Create a magnet uri object from a magnet URI string
/// @uri The magnet URI string to parse
/// @returns A magnet uri object
magnet_uri make_magnet_uri(std::string_view uri);


/// Create a magnet URI from a bittorrent metafile
/// @m The Metafile for which to create the magnet URI.
/// @protocol Which infohashes to include when m is a hybrid torrent.
/// @returns A magnet uri object
magnet_uri make_magnet_uri(const metafile& m, protocol protocol = protocol::v1 );


}
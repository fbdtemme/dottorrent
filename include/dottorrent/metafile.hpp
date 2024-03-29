//
// Created by fbdtemme on 07/07/19.
//
#pragma once

#include <compare>
#include <optional>
#include <vector>
#include <numeric>
#include <set>
#include <algorithm>
#include <bitset>
#include <regex>
#include <string_view>
#include <chrono>
#include <filesystem>
#include <unordered_set>
#include <string_view>
#include <compare>

#include <gsl-lite/gsl-lite.hpp>

#include <bencode/bencode.hpp>
#include <bencode/traits/vector.hpp>
#include <bencode/traits/string.hpp>
#include <bencode/traits/string_view.hpp>
#include <bencode/traits/unordered_set.hpp>

#include "dottorrent/dht_node.hpp"
#include "dottorrent/file_storage.hpp"
#include "dottorrent/announce_url.hpp"
#include "dottorrent/info_hash.hpp"
#include "dottorrent/announce_url_list.hpp"
#include "dottorrent/serialization/all.hpp"

namespace dottorrent {

class metafile
{
public:
    metafile() = default;

    metafile(const metafile&) = default;

    metafile(metafile&&) noexcept = default;

    metafile& operator=(const metafile&) = default;

    metafile& operator=(metafile&&) noexcept = default;

    const announce_url_list& trackers() const;

    announce_url_list& trackers();

    void add_tracker(announce_url announce);

    void add_tracker(std::string_view url, std::optional<std::size_t> tier = std::nullopt);

    void remove_tracker(const announce_url& announce);

    void remove_tracker(std::string_view url, std::optional<std::size_t> tier = std::nullopt);

    void clear_trackers();

    const std::vector<std::string>& http_seeds() const;

    std::vector<std::string>& http_seeds();

    void add_http_seed(std::string_view url);

    void remove_http_seed(std::string_view url);

    void clear_http_seeds();

    const std::vector<std::string>& web_seeds() const;

    void add_web_seed(std::string_view url);

    void remove_web_seed(std::string_view url);

    void clear_web_seeds();

    const std::vector<dht_node>& dht_nodes() const;

    void add_dht_node(std::string_view url, uint16_t port);

    void add_dht_node(const dht_node& node);

    void add_dht_node(dht_node&& node);

    void remove_dht_node(std::string_view url, uint16_t port);

    void clear_dht_nodes();

    const std::string&  name() const;

    void set_name(std::string_view name);

    const std::string& comment() const;

    void set_comment(std::string_view comment);

    const std::string& created_by() const;

    void set_created_by(std::string_view created_by);

    std::chrono::seconds creation_date() const;

    void set_creation_date(std::time_t time);

    template <typename Duration>
    void set_creation_date(std::chrono::time_point<std::chrono::system_clock, Duration> time)
    { creation_date_ = std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count(); }

    const std::unordered_set<info_hash>& similar_torrents() const;

    void add_similar_torrent(const info_hash& similar_torrent);

    void remove_similar_torrent(const info_hash& similar_torrent);

    void clear_similar_torrents();

    const std::unordered_set<std::string>& collections() const;

    void add_collection(std::string_view collection);

    void remove_collection(std::string_view collection);

    void clear_collections();

    const std::string& source() const;

    void set_source(std::string_view source);

    bool is_cross_seeding_enabled() const noexcept;

    // Add a hash of the announce urls to the info dict
    void enable_cross_seeding(bool flag = true) noexcept;

    bool is_private() const noexcept;

    void set_private(bool flag = true) noexcept;

    const file_storage& storage() const noexcept;

    file_storage& storage() noexcept;

    std::size_t piece_size() const noexcept;

    std::size_t piece_count() const noexcept;

    std::size_t total_file_size() const noexcept;

    std::size_t total_regular_file_size() const noexcept;

    const std::map<std::string, bencode::bvalue>& other_info_fields() const noexcept;

    std::map<std::string, bencode::bvalue>& other_info_fields() noexcept;

    bool operator==(const metafile& other) const = default;

private:
    enum protocol protocol_;
    file_storage storage_;

    announce_url_list announce_list_;
    std::vector<std::string> http_seeds_;
    std::vector<std::string> web_seeds_;
    std::vector<dht_node> dht_nodes_;

    std::string name_;
    std::string comment_;
    std::uint64_t creation_date_ = 0;
    std::string created_by_;
    bool private_ = false;

    std::string source_;
    bool enable_cross_seeding_ = false;
    std::map<std::string, bencode::bvalue> other_info_fields_;

    std::unordered_set<info_hash> similar_torrents_;
    std::unordered_set<std::string> collections_;
};

sha1_hash info_hash_v1(const metafile& m);

sha256_hash info_hash_v2(const metafile& m);

sha1_hash truncate_v2_hash(sha256_hash);

// Read functions

metafile read_metafile(std::istream &is);

metafile read_metafile(std::string_view buffer);

metafile read_metafile(std::span<const std::byte> buffer);

// Write functions

std::string write_metafile(const metafile& m, protocol protocol_version = protocol::v1);

void write_metafile_to(std::ostream &os, const metafile& m, protocol protocol_version = protocol::v1);

// load/save functions

metafile load_metafile(const std::filesystem::path& path);

void save_metafile(const std::filesystem::path& path, const metafile& m,
                   protocol protocol_version = protocol::v1);

std::string serialize_json(const metafile& m, protocol protocol_version = protocol::v1);

void serialize_json_to(std::ostream& os, const metafile& m,
                       protocol protocol_version = protocol::v1);

} // namesapce dottorrent
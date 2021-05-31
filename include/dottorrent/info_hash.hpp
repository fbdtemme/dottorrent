#pragma once
#include <variant>
#include <compare>
#include <optional>
#include <string_view>

#include <dottorrent/hex.hpp>
#include <dottorrent/hash.hpp>
#include <dottorrent/general.hpp>

namespace dottorrent {

/// Class storing both v1 and v2 infohashes.
class info_hash
{
public:
    info_hash(const sha1_hash& v1_hash)                    // NOLINT(google-explicit-constructor)
            : protocol_version_(protocol::v1)
            , v1_hash_(v1_hash)
            , v2_hash_()
    {}

    info_hash(const sha256_hash& v2_hash)                  // NOLINT(google-explicit-constructor)
            : protocol_version_(protocol::v2)
            , v1_hash_()
            , v2_hash_(v2_hash)
    {}

    info_hash(sha1_hash v1_hash, sha256_hash v2_hash)
            : protocol_version_(protocol::hybrid)
            , v1_hash_(v1_hash)
            , v2_hash_(v2_hash)
    {}

    info_hash(protocol version, std::string_view data)
        : protocol_version_(version)
    {
        if (version == protocol::v1 && data.size() == sha1_hash::size_bytes) {
            v1_hash_ = sha1_hash(data);
        }
        else if (version == protocol::v2 && data.size() == sha256_hash::size_bytes) {
            v2_hash_ = sha256_hash(data);
        }
        else {
            throw std::invalid_argument("invalid infohash length");
        }
    }

    info_hash(std::string_view v1_data, std::string_view v2_data)
            : protocol_version_(protocol::hybrid)
    {
        if (v1_data.size() == sha1_hash::size_bytes) {
            v1_hash_ = sha1_hash(v1_data);
        } else {
            throw std::invalid_argument("invalid v1 infohash length");
        }
        if (v2_data.size() == sha256_hash::size_bytes) {
            v2_hash_ = sha256_hash(v2_data);
        } else {
            throw std::invalid_argument("invalid v2 infohash length");
        }
    }

    static info_hash from_hex(protocol version, std::string_view hex_data) {
        Expects(version != protocol::hybrid);

        if (version == protocol::v1) {
            return info_hash(make_hash_from_hex<sha1_hash>(hex_data));
        }
        if (version == protocol::v2) {
            return info_hash(make_hash_from_hex<sha256_hash>(hex_data));
        }
        throw std::invalid_argument("invalid protocol version");
    }

    static info_hash from_hex(std::string_view v1_hex_data, std::string_view v2_hex_data)
    {
        auto v1_hash = make_hash_from_hex<sha1_hash>(v1_hex_data);
        auto v2_hash = make_hash_from_hex<sha256_hash>(v2_hex_data);
        return info_hash(v1_hash, v2_hash);
    }

    [[nodiscard]]
    protocol version() const noexcept
    {
        return protocol_version_;
    }

    std::string_view get_binary(protocol protocol_version) const
    {
        Expects(protocol_version != protocol::hybrid);

        switch (protocol_version) {
        case protocol::v1:
            return std::string_view(*v1_hash_);
        case protocol::v2:
            return std::string_view(*v2_hash_);
        default:
            throw std::invalid_argument("only v1 or v2 allowed");
        }
    }

    [[nodiscard]]
    std::string get_hex(enum protocol protocol_version) const
    {
        std::string_view binary = get_binary(protocol_version);
        return to_hexadecimal_string(std::span(reinterpret_cast<const std::byte*>(binary.data()), binary.size()));
    }

    const sha1_hash& v1() const noexcept
    {
        return *v1_hash_;
    }

    const sha256_hash& v2() const noexcept
    {
        return *v2_hash_;
    }

    [[nodiscard]]
    sha1_hash truncated_v2() const noexcept
    {
        return sha1_hash(std::string_view(*v2_hash_).substr(0, sha1_hash::size_bytes));
    }

    // Infohashes are only equal if they contain the exact same informatino.
    // V1 infohash of a hybrid torrent is not equal to an infohash of
    // the same torrent containing both the v1 and v2 infohash.
    bool operator==(const info_hash& other) const noexcept
    {
        return (protocol_version_ == other.protocol_version_) &&
               (v1_hash_ == other.v1_hash_) &&
               (v2_hash_ == other.v2_hash_);
    }

    std::strong_ordering operator<=>(const info_hash& other) const noexcept
    {
        if (protocol_version_ != other.protocol_version_) {
            return protocol_version_ <=> other.protocol_version_;
        }

        if (protocol_version_ == protocol::v1) {
            return v1_hash_ <=> other.v1_hash_;
        }
        return v2_hash_ <=> other.v2_hash_;
    }


private:
    protocol protocol_version_;
    std::optional<sha1_hash> v1_hash_;
    std::optional<sha256_hash> v2_hash_;
};

}

namespace std
{
template <>
struct hash<dottorrent::info_hash>
{
    std::size_t operator()(const dottorrent::info_hash& h) const noexcept
    {
        switch (h.version()) {
        case dottorrent::protocol::v1: {
            return std::hash<std::string_view>{}(h.get_binary(dottorrent::protocol::v1));
        }
        case dottorrent::protocol::v2: {
            return std::hash<std::string_view>{}(h.get_binary(dottorrent::protocol::v2));
        }
        case dottorrent::protocol::hybrid: {
            return std::hash<std::string_view>{}(h.get_binary(dottorrent::protocol::v1)) |
                   std::hash<std::string_view>{}(h.get_binary(dottorrent::protocol::v1));
        }
        default:
            return 0;
        }
    }
};
}
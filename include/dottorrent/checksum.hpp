#pragma once

#include <span>
#include <string>
#include <compare>
#include <ranges>

#include "dottorrent/hash.hpp"

namespace dottorrent {

namespace rng = std::ranges;

/// Checksum interface.
struct checksum
{
    virtual auto algorithm() const -> hash_function = 0;
    virtual auto name() const -> std::string_view = 0;
    virtual auto size() const -> std::size_t = 0;
    virtual auto value() const -> std::span<const std::byte> = 0;
    virtual auto value() -> std::span<std::byte> = 0;

    virtual auto hex_string() -> std::string
    {
        return detail::make_hex_string(value());
    }

    virtual ~checksum() noexcept = default;

    constexpr bool operator==(const checksum& other) const noexcept
    {
        if (this->name() != other.name()) return false;
        return rng::equal(this->value(), other.value());
    }

    constexpr std::partial_ordering operator<=>(const checksum& other) const noexcept
    {
        if (this->name() != other.name()) return std::partial_ordering::unordered;
        return std::lexicographical_compare_three_way(
                std::begin(this->value()), std::end(this->value()),
                std::begin(other.value()), std::end(other.value()));
    }
};


/// @brief Checksum class
class generic_checksum : public checksum
{
public:
    using value_type = std::vector<std::byte>;

    explicit generic_checksum(std::string_view key)
            : data_()
            , key_(key)
    {}

    generic_checksum(std::string_view key, std::span<const std::byte> value)
            : data_(std::begin(value), std::end(value))
            , key_(key)
    { }

    generic_checksum(std::string_view key, std::span<const char> value)
            : data_()
            , key_(key)
    {
        data_.reserve(value.size());
        std::transform(std::begin(value), std::end(value), std::begin(data_),
                [](char i) { return std::byte(i); });
    }

    /// Generic hash functions do not have a known hash_function enum value.
    auto algorithm() const -> hash_function override
    { return static_cast<hash_function>(0); }

    auto name() const -> std::string_view override
    { return key_; }

    auto size() const -> std::size_t override
    { return data_.size(); }

    auto value() const -> std::span<const std::byte> override
    { return std::span<const std::byte>(data_); }

    auto value() -> std::span<std::byte> override
    { return std::span<std::byte>(data_); }

private:
    value_type data_;
    std::string key_;
};


template <hash_type T>
class basic_checksum : public checksum
{
public:
    using storage_type = T;

    template <typename... Args>
    requires std::is_constructible_v<storage_type, Args...>
    explicit basic_checksum(Args... args)
            : data_(std::forward<Args>(args)...)
    { };

    auto algorithm() const -> hash_function override
    { return storage_type::algorithm; }

    auto name() const -> std::string_view override
    { return to_string(storage_type::algorithm); }

    auto size() const -> std::size_t override
    { return data_.size(); }

    auto value() const -> std::span<const std::byte> override
    { return std::span<const std::byte>(data_); }

    auto value() -> std::span<std::byte> override
    { return std::span<std::byte>(data_); }

private:
    storage_type data_;
};


#define DOTTORRENT_CHECKSUM_CLASS_TEMPLATE(NAME)                            \
using NAME##_checksum = basic_checksum<NAME##_hash>;                        \

DOTTORRENT_CHECKSUM_CLASS_TEMPLATE(md5)
DOTTORRENT_CHECKSUM_CLASS_TEMPLATE(sha1)
DOTTORRENT_CHECKSUM_CLASS_TEMPLATE(sha256)
DOTTORRENT_CHECKSUM_CLASS_TEMPLATE(sha512)
DOTTORRENT_CHECKSUM_CLASS_TEMPLATE(sha3_256)
DOTTORRENT_CHECKSUM_CLASS_TEMPLATE(sha3_512)
DOTTORRENT_CHECKSUM_CLASS_TEMPLATE(blake2s)
DOTTORRENT_CHECKSUM_CLASS_TEMPLATE(blake2b)


using checksum_types = std::tuple<
        md5_checksum,
        sha1_checksum,
        sha256_checksum,
        sha512_checksum,
        sha3_256_checksum,
        sha3_512_checksum,
        blake2s_checksum,
        blake2b_checksum >;

/// Create a checksum from a runtime string key
inline auto make_checksum(std::string_view key, std::span<const std::byte> value) -> std::unique_ptr<checksum>
{
    if (key == to_string(hash_function::md5))
        return std::make_unique<md5_checksum>(value);

    if (key == to_string(hash_function::sha1))
        return std::make_unique<sha1_checksum>(value);

    if (key == to_string(hash_function::sha256))
        return std::make_unique<sha256_checksum>(value);

    if (key == to_string(hash_function::sha512))
        return std::make_unique<sha512_checksum>(value);

    if (key == to_string(hash_function::sha3_256))
        return std::make_unique<sha3_256_checksum>(value);

    if (key == to_string(hash_function::sha3_512))
        return std::make_unique<sha3_512_checksum>(value);

    if (key == to_string(hash_function::blake2s))
        return std::make_unique<blake2s_checksum>(value);

    if (key == to_string(hash_function::blake2b))
        return std::make_unique<blake2b_checksum>(value);

    return std::make_unique<generic_checksum>(key, value);
}

/// Create a checksum from a runtime string key
inline auto make_checksum(std::string_view key) -> std::unique_ptr<checksum>
{
    if (key == to_string(hash_function::md5))
        return std::make_unique<md5_checksum>();

    if (key == to_string(hash_function::sha1))
        return std::make_unique<sha1_checksum>();

    if (key == to_string(hash_function::sha256))
        return std::make_unique<sha256_checksum>();

    if (key == to_string(hash_function::sha512))
        return std::make_unique<sha512_checksum>();

    if (key == to_string(hash_function::sha3_256))
        return std::make_unique<sha3_256_checksum>();

    if (key == to_string(hash_function::sha3_512))
        return std::make_unique<sha3_512_checksum>();

    if (key == to_string(hash_function::blake2s))
        return std::make_unique<blake2s_checksum>();

    if (key == to_string(hash_function::blake2b))
        return std::make_unique<blake2b_checksum>();

    return std::make_unique<generic_checksum>(key);
}

inline auto make_checksum(hash_function f) -> std::unique_ptr<checksum>
{
    switch (f) {
        case hash_function::md5:      return std::make_unique<md5_checksum>();
        case hash_function::sha1:     return std::make_unique<sha1_checksum>();
        case hash_function::sha256:   return std::make_unique<sha256_checksum>();
        case hash_function::sha512:   return std::make_unique<sha512_checksum>();
        case hash_function::sha3_256: return std::make_unique<sha3_256_checksum>();
        case hash_function::sha3_512: return std::make_unique<sha3_512_checksum>();
        case hash_function::blake2s:  return std::make_unique<blake2s_checksum>();
        case hash_function::blake2b:  return std::make_unique<blake2b_checksum>();
        default: throw std::invalid_argument("invalid hash function");
    }
}

inline auto make_checksum(hash_function f, std::span<const std::byte> value) -> std::unique_ptr<checksum>
{
    switch (f) {
    case hash_function::md5:      return std::make_unique<md5_checksum>(value);
    case hash_function::sha1:     return std::make_unique<sha1_checksum>(value);
    case hash_function::sha256:   return std::make_unique<sha256_checksum>(value);
    case hash_function::sha512:   return std::make_unique<sha512_checksum>(value);
    case hash_function::sha3_256: return std::make_unique<sha3_256_checksum>(value);
    case hash_function::sha3_512: return std::make_unique<sha3_512_checksum>(value);
    case hash_function::blake2s:  return std::make_unique<blake2s_checksum>(value);
    case hash_function::blake2b:  return std::make_unique<blake2b_checksum>(value);
    default: throw std::invalid_argument("invalid hash function");
    }
}

inline std::unique_ptr<checksum> make_checksum(std::string_view key, std::string_view value)
{
    return make_checksum(key, {reinterpret_cast<const std::byte*>(value.data()), value.size()});
}

inline std::unique_ptr<checksum> make_checksum(hash_function f, std::string_view value)
{
    return make_checksum(f, {reinterpret_cast<const std::byte*>(value.data()), value.size()});
}

inline std::unique_ptr<checksum> make_checksum_from_hex(std::string_view key, std::string_view hex_string)
{
    std::vector<std::byte> data {};
    detail::parse_hexdigest_to(std::back_inserter(data), hex_string);
    if (data.size() != hex_string.size() / 2)
        throw std::invalid_argument(
                "invalid digest: contains non-hexadecimal characters");

    return make_checksum(key, std::move(data));
}


inline std::unique_ptr<checksum> make_checksum_from_hex(hash_function key, std::string_view hex_string)
{
    std::vector<std::byte> data {};
    detail::parse_hexdigest_to(std::back_inserter(data), hex_string);
    if (data.size() != hex_string.size() / 2)
        throw std::invalid_argument(
                "invalid digest: contains non-hexadecimal characters");

    return make_checksum(key, std::move(data));
}

} // namespace dottorrent
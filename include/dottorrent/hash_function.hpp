#pragma once
#include <cstdint>
#include <string_view>
#include <optional>
#include <stdexcept>
#include <algorithm>

namespace dottorrent {

using namespace std::string_view_literals;

enum class hash_function : std::uint8_t {
    unknown = 0,
    md5,
    sha1,
    sha256,
    sha512,
    sha3_256,
    sha3_512,
    blake2s,
    blake2b,
};

/// List of supported algorithms to calculate checksums.
constexpr std::array supported_hash_functions {
        hash_function::md5,
        hash_function::sha1,
        hash_function::sha256,
        hash_function::sha512,
        hash_function::sha3_256,
        hash_function::sha3_512,
        hash_function::blake2s,
        hash_function::blake2b,
};

constexpr std::string_view to_string(hash_function f)
{
    switch (f) {
    case hash_function::md5:      return "md5"sv;
    case hash_function::sha1:     return "sha1"sv;
    case hash_function::sha256:   return "sha256"sv;
    case hash_function::sha512:   return "sha512"sv;
    case hash_function::sha3_256: return "sha3-256"sv;
    case hash_function::sha3_512: return "sha3-512"sv;
    case hash_function::blake2s:  return "blake2s"sv;
    case hash_function::blake2b:  return "blake2b"sv;
    default:
        throw std::invalid_argument("invalid hash_function");
    }
}

constexpr bool is_hash_function_name(std::string_view s)
{
    return std::apply([=](auto... v){
        return ((to_string(v) == s) || ... );
    }, supported_hash_functions);
}

constexpr auto make_hash_function(std::string_view s) -> std::optional<hash_function>
{
    for (auto func: supported_hash_functions) {
        if (s == to_string(func)) return func;
    }
    return std::nullopt;
}

}
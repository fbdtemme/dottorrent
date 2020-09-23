#pragma once

#include "dottorrent/hash_function.hpp"
#include "dottorrent/bitmask_operators.hpp"

namespace dottorrent {

/// Available checksums that can be added to a file_entry.
enum class checksum_options {
    none = 0U << 0U,
    md5 = 1U << 0U,
    sha1 = 1U << 1U,
    sha256 = 1U << 2U,
    sha512 = 1U << 3U,
//    sha3_256 = 1U << 4U,
//    sha3_512 = 1U << 5U,
//    blake2s = 1U << 6U,
//    blake2b = 1U << 7U
};



} // namespace dottorrent
DOTTORRENT_ENABLE_BITMASK_OPERATORS(dottorrent::checksum_options);
namespace dottorrent {


constexpr auto to_checksum_option(hash_function f) -> checksum_options
{
    switch (f) {
        case hash_function::md5:      return checksum_options::md5;
        case hash_function::sha1:     return checksum_options::sha1;
        case hash_function::sha256:   return checksum_options::sha256;
        case hash_function::sha512:   return checksum_options::sha512;
//        case hash_function::sha3_256: return checksum_options::sha3_256;
//        case hash_function::sha3_512: return checksum_options::sha3_512;
//        case hash_function::blake2s:  return checksum_options::blake2s;
//        case hash_function::blake2b:  return checksum_options::blake2b;
        default:
            throw std::invalid_argument("invalid hash function");
    }
}

}// namespace dottorrent
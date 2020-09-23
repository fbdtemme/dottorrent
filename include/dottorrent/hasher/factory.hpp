#pragma once

#include <memory>

#include "dottorrent/hasher/base.hpp"
#include "dottorrent/hasher/md5.hpp"
#include "dottorrent/hasher/sha1.hpp"
#include "dottorrent/hasher/sha256.hpp"
#include "dottorrent/hasher/sha512.hpp"


namespace dottorrent {


/// Return a polymorphic hasher for given algorithm
inline std::unique_ptr<hasher> make_hasher(hash_function f)
{
    switch (f) {
    case hash_function::md5:      return std::make_unique<md5_hasher>();
    case hash_function::sha1:     return std::make_unique<sha1_hasher>();
    case hash_function::sha256:   return std::make_unique<sha256_hasher>();
    case hash_function::sha512:   return std::make_unique<sha512_hasher>();
//    case hash_function::sha3_256: return std::make_unique<sha3_256_hasher>();
//    case hash_function::sha3_512: return std::make_unique<sha3_512_hasher>();
//    case hash_function::blake2s:  return std::make_unique<blake2s_hasher>();
//    case hash_function::blake2b:  return std::make_unique<blake2b_hasher>();
    default:
        throw std::invalid_argument("invalid hash_function");
    }
}

}
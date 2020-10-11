#pragma once

#include <memory>

#include "dottorrent/hash_function_traits.hpp"
#include "dottorrent/hasher/hasher.hpp"

#ifdef DOTTORRENT_USE_OPENSSL
#include "dottorrent/hasher/openssl_hasher.hpp"
#endif

#ifdef DOTTORRENT_USE_GCRYPT
#include "dottorrent/hasher/gcrypt_hasher.hpp"
#endif


namespace dottorrent {

/// Return a polymorphic hasher for given algorithm
inline std::unique_ptr<hasher> make_hasher(hash_function f)
{
#ifdef DOTTORRENT_USE_OPENSSL
    return std::make_unique<openssl_hasher>(f);
#endif
#ifdef DOTTORRENT_USE_GCRYPT
    return std::make_unique<gcrypt_hasher>(f);
#endif

}

template <hash_function F>
inline typename hash_function_traits<F>::hash_type
make_hash(std::span<const std::byte> data)
{
    using hash_t = typename hash_function_traits<F>::hash_type;

#ifdef DOTTORRENT_USE_OPENSSL
    auto hasher =  std::make_unique<openssl_hasher>(F);
#endif
#ifdef DOTTORRENT_USE_GCRYPT
    auto hasher =  std::make_unique<gcrypt_hasher>(F);
#endif

    hash_t result {};
    hasher->update(data);
    hasher->finalize_to(result);
    return result;
}

}

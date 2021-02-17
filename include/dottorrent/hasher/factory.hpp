#pragma once

#include <memory>
#include <unordered_set>

#include "dottorrent/hash_function_traits.hpp"
#include "dottorrent/hasher/single_buffer_hasher.hpp"
#include "dottorrent/hasher/multi_buffer_hasher.hpp"

#ifdef DOTTORRENT_USE_OPENSSL
#include "dottorrent/hasher/openssl_hasher.hpp"

#endif
#ifdef DOTTORRENT_USE_GCRYPT
#include "dottorrent/hasher/gcrypt_hasher.hpp"
#endif
#ifdef DOTTORRENT_USE_WINCNG
#include "dottorrent/hasher/wincng_hasher.hpp"
#endif

#if defined(DOTTORRENT_USE_ISAL)
#include "dottorrent/hasher/isal_multi_buffer_hasher.hpp"
#include "dottorrent/hasher/backends/isal.hpp"
#endif


namespace dottorrent {

/// Return a polymorphic hasher for given algorithm
inline std::unique_ptr<single_buffer_hasher> make_hasher(hash_function f)
{
#if defined(DOTTORRENT_USE_OPENSSL)
    return std::make_unique<openssl_hasher>(f);
#elif defined(DOTTORRENT_USE_GCRYPT)
    return std::make_unique<gcrypt_hasher>(f);
#elif defined(DOTTORRENT_USE_WINCNG)
    return std::make_unique<wincng_hasher>(f);
#else
    #error "No cryptographic library specified"
#endif
}


/// Return a polymorphic hasher for given algorithm
inline std::unique_ptr<multi_buffer_hasher> make_multi_buffer_hasher(hash_function f)
{
#if defined(DOTTORRENT_USE_ISAL)
    return std::make_unique<isal_multi_buffer_hasher>(f);
#endif
    return nullptr;
}


template <hash_function F>
inline typename hash_function_traits<F>::hash_type
make_hash(std::span<const std::byte> data)
{
    using hash_t = typename hash_function_traits<F>::hash_type;

#if defined(DOTTORRENT_USE_OPENSSL)
    auto hasher = std::make_unique<openssl_hasher>(F);
#elif defined(DOTTORRENT_USE_GCRYPT)
    auto hasher = std::make_unique<gcrypt_hasher>(F);
#elif defined(DOTTORRENT_USE_WINCNG)
    auto hasher = std::make_unique<wincng_hasher>(F);
#else
    #error "No cryptographic library specified"
#endif

    hash_t result {};
    hasher->update(data);
    hasher->finalize_to(result);
    return result;
}

inline const std::unordered_set<hash_function>&
hasher_supported_algorithms() noexcept
{
#if defined(DOTTORRENT_USE_OPENSSL)
    return openssl_hasher::supported_algorithms();
#elif defined(DOTTORRENT_USE_GCRYPT)
    return gcrypt_hasher::supported_algorithms();
#elif defined(DOTTORRENT_USE_WINCNG)
    return wincng_hasher::supported_algorithms();
#else
    #error "No cryptographic library specified!"
#endif
}

}

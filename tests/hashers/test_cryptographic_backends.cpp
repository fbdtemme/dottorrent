//
// Created by fbdtemme on 8/1/21.
//

#include <catch2/catch_all.hpp>
#include <dottorrent/hasher/backend_info.hpp>

namespace dt = dottorrent;


TEST_CASE("test crypto backends versions")
{
    auto versions = dt::cryptographic_backends();

#if defined(DOTTORRENT_USE_OPENSSL)
    CHECK(versions.contains("openssl"));
#endif
#if defined(DOTTORRENT_USE_ISAL_CRYPTO)
    CHECK(versions.contains("isa-l_crypto"));
#endif
#if defined(DOTTORRENT_USE_GCRYPT)
    CHECK(versions.contains("gcrypt"));
#endif
#if defined(DOTTORRENT_USE_WOLFSSL)
    CHECK(versions.contains("wolfssl"));
#endif
#if defined(DOTTORRENT_USE_WINCNG)
    CHECK(versions.contains("wincng"));
#endif
}
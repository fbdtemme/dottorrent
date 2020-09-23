#pragma once

#include <array>
#include <string_view>
#include <span>

#if defined(DOTTORRENT_USE_OPENSSL)
    #include <openssl/crypto.h>
    #include <openssl/sha.h>
#elif defined(DOTTORRENT_USE_CRYPTOPP)
    #include <crypto++/sha.h>
#elif defined(DOTTORRENT_USE_BOTAN)
    #include <botan/hash.h>
    #include <botan/sha160.h>
#endif

#include "dottorrent/hash.hpp"
#include "dottorrent/hasher/base.hpp"

namespace dottorrent {

class sha1_hasher : public hasher
{
public:
    using hash_type = sha1_hash;

    explicit sha1_hasher();

    void update(std::span<const std::byte> data) override;

    void finalize_to(std::span<std::byte> out) override;

    hash_type finalize();

private:

#if defined(DOTTORRENT_USE_OPENSSL)
    SHA_CTX context_;
#elif defined(DOTTORRENT_USE_CRYPTOPP)
    CryptoPP::SHA1 context_;
#elif defined(DOTTORRENT_USE_BOTAN)
    Botan::SHA_160 context_;
#endif
};

auto make_sha1_hash(std::string_view data) -> sha1_hash;

} // namespace dottorrent
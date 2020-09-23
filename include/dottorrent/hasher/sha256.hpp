#pragma once

#include <array>
#include <string_view>
#include <span>

#if defined(DOTTORRENT_CRYPTO_OPENSSL)
    #include <openssl/crypto.h>
    #include <openssl/sha.h>
#elif defined(DOTTORRENT_CRYPTO_CRYPTOPP)
    #include <crypto++/sha.h>
#elif defined(DOTTORRENT_CRYPTO_BOTAN)
    #include <botan/hash.h>
    #include <botan/sha2_32.h>
#endif

#include "dottorrent/hash.hpp"
#include "dottorrent/hasher/base.hpp"


namespace dottorrent {

class sha256_hasher : public hasher
{
public:
    using hash_type = sha256_hash;

    explicit sha256_hasher();

    void update(std::span<const std::byte> data) override ;

    void finalize_to(std::span<std::byte> out) override;

    hash_type finalize();

private:

#if defined(DOTTORRENT_CRYPTO_OPENSSL)
    SHA256_CTX context_;
#elif defined(DOTTORRENT_CRYPTO_CRYPTOPP)
    CryptoPP::SHA256 context_;
#elif defined(DOTTORRENT_CRYPTO_BOTAN)
    Botan::SHA_256 context_;
#endif
};

auto make_sha256_hash(std::string_view data) -> sha256_hash;


} // namespace dottorrent
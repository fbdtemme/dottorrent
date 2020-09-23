#pragma once

#include <array>
#include <string_view>
#include <span>

#if defined(DOTTORRENT_CRYPTO_OPENSSL)
    #include <openssl/crypto.h>
    #include <openssl/md5.h>
#elif defined(DOTTORRENT_CRYPTO_CRYPTOPP)
    #include <crypto++/md5.h>
#elif defined(DOTTORRENT_CRYPTO_BOTAN)
    #include <botan/hash.h>
    #include <botan/sha2_32.h>
#endif

#include "dottorrent/hasher/base.hpp"
#include "dottorrent/hash.hpp"

namespace dottorrent {

class md5_hasher : public hasher
{
public:
    using hash_type = md5_hash;

    explicit md5_hasher();

    void update(std::span<const std::byte> data) override;

    void finalize_to(std::span<std::byte> out) override;

    hash_type finalize();

private:

#if defined(DOTTORRENT_CRYPTO_OPENSSL)
    MD5_CTX context_;
#elif defined(DOTTORRENT_CRYPTO_CRYPTOPP)
    CryptoPP::Weak::MD5 context_;
#elif defined(DOTTORRENT_CRYPTO_BOTAN)
    Botan::MD5 context_;
#endif
};


} // namespace dottorrent
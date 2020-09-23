#pragma once
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
    #include <botan/sha2_64.h>
#endif

#include "dottorrent/hash.hpp"
#include "dottorrent/hasher/base.hpp"


namespace dottorrent {

class sha512_hasher : public hasher
{
public:
    using hash_type = sha512_hash;

    explicit sha512_hasher();

    void update(std::span<const std::byte> data) override ;

    void finalize_to(std::span<std::byte> out) override;

    hash_type finalize();

private:

#if defined(DOTTORRENT_USE_OPENSSL)
    SHA512_CTX context_;
#elif defined(DOTTORRENT_USE_CRYPTOPP)
    CryptoPP::SHA512 context_;
#elif defined(DOTTORRENT_USE_BOTAN)
    Botan::SHA_512 context_;
#endif
};

auto make_sha512_hash(std::string_view data) -> sha512_hash;


} // namespace dottorrent
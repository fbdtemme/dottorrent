#pragma once

#include "dottorrent/hash_function.hpp"
#include "dottorrent/hasher/hasher.hpp"
#include "dottorrent/hasher/backends/openssl.hpp"

namespace dottorrent {

namespace detail {

constexpr openssl::message_digest_algorithm get_openssl_algorithm(hash_function function)
{
    using namespace openssl;

    switch (function) {
    case hash_function::md4:          return message_digest_algorithm::md4;
    case hash_function::md5:          return message_digest_algorithm::md5;
    case hash_function::blake2b_512:   return message_digest_algorithm::blake2b512;
    case hash_function::blake2s_256:   return message_digest_algorithm::blake2s256;
    case hash_function::sha1:         return message_digest_algorithm::sha1;
    case hash_function::sha224:       return message_digest_algorithm::sha224;
    case hash_function::sha256:       return message_digest_algorithm::sha256;
    case hash_function::sha384:       return message_digest_algorithm::sha384;
    case hash_function::sha512:       return message_digest_algorithm::sha512;
    case hash_function::sha3_224:     return message_digest_algorithm::sha3_224;
    case hash_function::sha3_256:     return message_digest_algorithm::sha3_256;
    case hash_function::sha3_384:     return message_digest_algorithm::sha3_384;
    case hash_function::sha3_512:     return message_digest_algorithm::sha3_512;
    case hash_function::shake128:     return message_digest_algorithm::shake128;
    case hash_function::shake256:     return message_digest_algorithm::shake256;
    case hash_function::ripemd160:    return message_digest_algorithm::ripemd160;
    case hash_function::whirlpool:    return message_digest_algorithm::whirlpool;

    default:
        throw std::invalid_argument("No matching openssl algorithm.");
    }
}
}

class openssl_hasher : public hasher
{
public:
    explicit openssl_hasher(hash_function function)
            : digest_(detail::get_openssl_algorithm(function))
    {}

    void update(std::span<const std::byte> data) override
    {
        digest_.update(data);
    }

    void finalize_to(std::span<std::byte> out) override
    {
        digest_.finalize_to(out);
        digest_.reset();
    }

private:
    openssl::message_digest digest_;
};

}
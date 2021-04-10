#ifdef DOTTORRENT_USE_OPENSSL
#include <system_error>
#include "dottorrent/hasher/backends/openssl.hpp"

namespace openssl {

const std::error_category& openssl_crypto_category()
{
    static detail::openssl_crypto_category instance{};
    return instance;
}

namespace detail {

const ::EVP_MD* get_evp_message_digest(message_digest_algorithm algorithm)
{
    switch (algorithm) {
#ifndef OPENSSL_NO_MD2
    case message_digest_algorithm::md2: {
        return ::EVP_md2();
    }
#endif
#ifndef OPENSSL_NO_MD4
    case message_digest_algorithm::md4: {
        return ::EVP_md4();
    }
#endif
#ifndef OPENSSL_NO_MD5
    case message_digest_algorithm::md5: {
        return ::EVP_md5();
    }
#endif
#ifndef OPENSSL_NO_MD5
    case message_digest_algorithm::md5_sha1: {
        return ::EVP_md5_sha1();
    }
#endif
#ifndef OPENSSL_NO_BLAKE2
    case message_digest_algorithm::blake2b512: {
        return ::EVP_blake2b512();
    }
#endif
#ifndef OPENSSL_NO_BLAKE2
    case message_digest_algorithm::blake2s256: {
        return ::EVP_blake2s256();
    }
#endif
    case message_digest_algorithm::sha1:
        return ::EVP_sha1();
    case message_digest_algorithm::sha224:
        return ::EVP_sha224();
    case message_digest_algorithm::sha256:
        return ::EVP_sha256();
    case message_digest_algorithm::sha384:
        return ::EVP_sha384();
    case message_digest_algorithm::sha512:
        return ::EVP_sha512();
    case message_digest_algorithm::sha512_224:
        return ::EVP_sha512_224();
    case message_digest_algorithm::sha512_256:
        return ::EVP_sha512_256();
    case message_digest_algorithm::sha3_224:
        return ::EVP_sha3_224();
    case message_digest_algorithm::sha3_256:
        return ::EVP_sha3_256();
    case message_digest_algorithm::sha3_384:
        return ::EVP_sha3_384();
    case message_digest_algorithm::sha3_512:
        return ::EVP_sha3_512();
    case message_digest_algorithm::shake128:
        return ::EVP_shake128();
    case message_digest_algorithm::shake256:
        return ::EVP_shake256();
#ifndef OPENSSL_NO_MDC2
    case message_digest_algorithm::mdc2: {
        return ::EVP_mdc2();
    }
#endif
#ifndef OPENSSL_NO_RMD160
    case message_digest_algorithm::ripemd160: {
        return ::EVP_ripemd160();
    }
#endif
#ifndef OPENSSL_NO_WHIRLPOOL
    case message_digest_algorithm::whirlpool: {
        return ::EVP_whirlpool();
    }
#endif
# ifndef OPENSSL_NO_SM3
    case message_digest_algorithm::sm3: {
        return ::EVP_sm3();
    }
#endif
    default:
        return nullptr;
    }
}

const char* openssl_crypto_category::name() const noexcept {
    return "openssl-crypto";
}

std::string openssl_crypto_category::message(int value) const {
    const char* s = ::ERR_reason_error_string(value);
    return s ? s : "openssl-crypto error";
}
} // namespace detail


message_digest::message_digest(message_digest_algorithm algorithm, ::ENGINE* engine)
        : context_()
        , algorithm_context_(detail::get_evp_message_digest(algorithm))
        , engine_(engine)
{
    context_ = ::EVP_MD_CTX_new();
    if (context_ == nullptr) {
        throw std::system_error((int)ERR_get_error(), openssl_crypto_category());
    }
    ::EVP_MD_CTX_init(context_);

    if (algorithm_context_ == nullptr) {
        throw std::system_error(make_error_code(evp_errc::unsupported_algorithm));
    }
    if (::EVP_DigestInit_ex(context_, algorithm_context_, engine_) != 1) {
        throw std::system_error((int)ERR_get_error(), openssl_crypto_category());
    }
}

void message_digest::update(std::span<const std::byte> data) {
    ::EVP_DigestUpdate(context_, reinterpret_cast<const void*>(data.data()), data.size());
}

void message_digest::finalize_to(std::span<std::byte> digest) {
    auto digest_size = static_cast<unsigned int>(digest.size());
    ::EVP_DigestFinal_ex(context_, reinterpret_cast<unsigned char*>(digest.data()), &digest_size);
}

void message_digest::reset() {
    if (::EVP_DigestInit_ex(context_, algorithm_context_, engine_) != 1) {
        throw std::system_error((int)ERR_get_error(), openssl_crypto_category());
    }
}

message_digest::~message_digest() {
    ::EVP_MD_CTX_free(context_);
}
}

#endif
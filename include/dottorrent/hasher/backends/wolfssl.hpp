#pragma once

#include <type_traits>
#include <system_error>
#include <span>
#include <unordered_set>


#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/sha.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/sha512.h>
#include <wolfssl/wolfcrypt/sha3.h>
#include <wolfssl/wolfcrypt/md2.h>
#include <wolfssl/wolfcrypt/md4.h>
#include <wolfssl/wolfcrypt/md5.h>

#include "dottorrent/hasher/single_buffer_hasher.hpp"

namespace dottorrent {

namespace wolfssl {

template<typename CTX, auto Init, auto Update, auto Final>
class wolfssl_hasher_impl : public single_buffer_hasher
{
public:
    explicit wolfssl_hasher_impl()
            : context_()
    {
        Init(&context_);
    }

    void update(std::span<const std::byte> data)
    {
        Update(&context_, reinterpret_cast<const byte*>(data.data()), data.size());
    }

    void finalize_to(std::span<std::byte> digest)
    {
        Final(&context_, reinterpret_cast<byte*>(digest.data()));
        reset();
    }

    void reset()
    {
        Init(&context_);
    }

private:
    CTX context_;
};

#ifdef WOLFSSL_MD2
extern template class wolfssl_hasher_impl<Md2, wc_InitMd2, wc_Md2Update, wc_Md2Final>;
using md2_hasher    = wolfssl_hasher_impl<Md2, wc_InitMd2, wc_Md2Update, wc_Md2Final>;
#endif

#ifndef NO_MD4
extern template class wolfssl_hasher_impl<Md4, wc_InitMd4, wc_Md4Update, wc_Md4Final>;
using md4_hasher    = wolfssl_hasher_impl<Md4, wc_InitMd4, wc_Md4Update, wc_Md4Final>;
#endif

extern template class wolfssl_hasher_impl<wc_Md5,wc_InitMd5, wc_Md5Update, wc_Md5Final>;
using md5_hasher    = wolfssl_hasher_impl<wc_Md5, wc_InitMd5, wc_Md5Update, wc_Md5Final>;

extern template class wolfssl_hasher_impl<wc_Sha, wc_InitSha, wc_ShaUpdate, wc_ShaFinal>;
using sha1_hasher   = wolfssl_hasher_impl<wc_Sha, wc_InitSha, wc_ShaUpdate, wc_ShaFinal>;

extern template class wolfssl_hasher_impl<wc_Sha256, wc_InitSha256, wc_Sha256Update, wc_Sha256Final>;
using sha256_hasher = wolfssl_hasher_impl<wc_Sha256, wc_InitSha256, wc_Sha256Update, wc_Sha256Final>;

extern template class wolfssl_hasher_impl<wc_Sha512, wc_InitSha512, wc_Sha512Update, wc_Sha512Final>;
using sha512_hasher = wolfssl_hasher_impl<wc_Sha512, wc_InitSha512, wc_Sha512Update, wc_Sha512Final>;

static const auto supported_hash_functions = std::unordered_set {
#ifdef WOLFSSL_MD2
        hash_function::md2,
#endif
#ifndef NO_MD4
        hash_function::md4,
#endif
        hash_function::md5,
        hash_function::sha1,
        hash_function::sha256,
        hash_function::sha512,
};

} // namespace wolfssl
} // namespace dottorrent
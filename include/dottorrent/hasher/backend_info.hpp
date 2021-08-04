#pragma once

#include <map>
#include <string>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#if defined(DOTTORRENT_USE_OPENSSL)
#include <openssl/crypto.h>
#endif

#if defined(DOTTORRENT_USE_ISAL)
#include <isa-l_crypto.h>
#endif

#if defined(DOTTORRENT_USE_GCRYPT)
#include <gcrypt.h>
#endif

#if defined(DOTTORRENT_USE_WOLFSSL)
#include <wolfssl/version.h>
#endif

#if defined(DOTTORRENT_USE_WINCNG)
#include <windows.h>
#include <ntstatus.h>
#include <bcrypt.h>
#endif

#define MAKE_VERSION_STRING(major, minor, patch) STR(major) "." STR(minor) "." STR(patch)

namespace dottorrent {

inline std::map<std::string, std::string>
cryptographic_backends()
{
    std::map<std::string, std::string> versions;

#if defined(DOTTORRENT_USE_OPENSSL)
    versions["openssl"] = OPENSSL_VERSION_TEXT;
#endif
#if defined(DOTTORRENT_USE_ISAL)
    versions["isa-l_crypto"] = MAKE_VERSION_STRING(ISAL_CRYPTO_MAJOR_VERSION, ISAL_CRYPTO_MINOR_VERSION, ISAL_CRYPTO_PATCH_VERSION);
#endif
#if defined(DOTTORRENT_USE_GCRYPT)
    versions["gcrypt"] = GCRYPT_VERSION;
#endif
#if defined(DOTTORRENT_USE_WOLFSSL)
    versions["wolfssl"] = LIBWOLFSSL_VERSION_STRING;
#endif
#if defined(DOTTORRENT_USE_WINCNG)
    versions["wincng"] = BCRYPT_HASH_INTERFACE_VERSION_1;
#endif

    return versions;
}

} // namespace dottorrent
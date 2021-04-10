#ifdef DOTTORRENT_USE_WOLFSSL
#include <system_error>
#include "dottorrent/hasher/backends/wolfssl.hpp"
namespace dottorrent::wolfssl
{

#ifdef WOLFSSL_MD2
template class wolfssl_hasher_impl<Md2, wc_InitMd2, wc_Md2Update, wc_Md2Final>;
#endif

#ifndef NO_MD4
template class wolfssl_hasher_impl<Md4, wc_InitMd4, wc_Md4Update, wc_Md4Final>;
#endif

template class wolfssl_hasher_impl<wc_Md5,wc_InitMd5, wc_Md5Update, wc_Md5Final>;
template class wolfssl_hasher_impl<wc_Sha, wc_InitSha, wc_ShaUpdate, wc_ShaFinal>;
template class wolfssl_hasher_impl<wc_Sha256, wc_InitSha256, wc_Sha256Update, wc_Sha256Final>;
template class wolfssl_hasher_impl<wc_Sha512, wc_InitSha512, wc_Sha512Update, wc_Sha512Final>;
}

#endif


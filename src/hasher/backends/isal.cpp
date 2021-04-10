#ifdef DOTTORRENT_USE_ISAL
#include <system_error>
#include "dottorrent/hasher/backends/isal.hpp"
namespace isal
{

template class multi_buffer_hasher_impl<
        MD5_HASH_CTX_MGR, MD5_HASH_CTX,
        &md5_ctx_mgr_init, &md5_ctx_mgr_submit, &md5_ctx_mgr_flush,
        MD5_DIGEST_NWORDS
>;

template class multi_buffer_hasher_impl<
        SHA1_HASH_CTX_MGR, SHA1_HASH_CTX,
        &sha1_ctx_mgr_init, &sha1_ctx_mgr_submit, &sha1_ctx_mgr_flush,
        SHA1_DIGEST_NWORDS
>;

template class multi_buffer_hasher_impl<
        SHA256_HASH_CTX_MGR, SHA256_HASH_CTX,
        &sha256_ctx_mgr_init, &sha256_ctx_mgr_submit, &sha256_ctx_mgr_flush,
        SHA256_DIGEST_NWORDS
>;

template class multi_buffer_hasher_impl<
        SHA512_HASH_CTX_MGR, SHA512_HASH_CTX,
        &sha512_ctx_mgr_init, &sha512_ctx_mgr_submit, &sha512_ctx_mgr_flush,
        SHA512_DIGEST_NWORDS
>;

}

#endif

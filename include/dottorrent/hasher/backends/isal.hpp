#pragma once
#include <vector>
#include <span>
#include <string_view>
#include <cstring>
#include <unordered_set>

#include "dottorrent/hasher/multi_buffer_hasher.hpp"
#include "dottorrent/hash_function.hpp"

#include <gsl-lite/gsl-lite.hpp>

#if defined __has_include
    #if __has_include(<isa-l_crypto/md5_mb.h>) \
            &&__has_include(<isa-l_crypto/sha256_mb.h>)\
            &&__has_include(<isa-l_crypto/sha1_mb.h>)  \
            &&__has_include(<isa-l_crypto/sha512_mb.h>)\
            &&__has_include(<isa-l_crypto/endian_helper.h>)
        #include <isa-l_crypto/md5_mb.h>
        #include <isa-l_crypto/sha1_mb.h>
        #include <isa-l_crypto/sha256_mb.h>
        #include <isa-l_crypto/sha512_mb.h>
        #include <isa-l_crypto/endian_helper.h>
    #else
#endif
    #include <md5_mb.h>
    #include <sha1_mb.h>
    #include <sha256_mb.h>
    #include <sha512_mb.h>
    #include <endian_helper.h>
#endif

#if defined(_WIN32) || defined(__MINGW32__)
#define USE_ALLIGED_MALLOC
#include <malloc.h>
#endif


namespace isal {

template <typename MGR, typename CTX, auto Init, auto Submit, auto Flush, std::size_t N_WORDS>
class multi_buffer_hasher_impl : public multi_buffer_hasher
{
    using user_data = struct { std::size_t index; };

public:
    multi_buffer_hasher_impl()
    {
        constexpr std::size_t alignment = 64;
        constexpr std::size_t n = ((sizeof(MGR) + alignment -1) / alignment) * alignment;

#ifdef USE_ALLIGED_MALLOC
        context_manager_ = static_cast<MGR*>(_aligned_malloc(n, alignment));
#else
        context_manager_ = static_cast<MGR*>(std::aligned_alloc(alignment, n));
#endif
        if (context_manager_ == NULL)
            throw std::runtime_error("could not allocate context manager");

        Init(context_manager_);
    }

    std::size_t submit(std::span<const std::byte> data) override
    {
        std::size_t job_id;
        CTX* hash_ctx = allocate_hash_context(job_id);
        Submit(context_manager_, hash_ctx, data.data(), data.size(), HASH_ENTIRE);
        return job_id;
    }

   void submit(std::size_t job_id, std::span<const std::byte> data) override
    {
        Expects(job_id < context_pool_.size());
        Submit(context_manager_, context_pool_[job_id], data.data(), data.size(), HASH_ENTIRE);
    }

    std::size_t submit_first(std::span<const std::byte> data) override
    {
        std::size_t job_idx;
        CTX* hash_ctx = allocate_hash_context(job_idx);
        Submit(context_manager_, hash_ctx, data.data(), data.size(), HASH_FIRST);
        return job_idx;
    }

    void submit_first(std::size_t job_id, std::span<const std::byte> data) override
    {
        Expects(job_id < context_pool_.size());
        Submit(context_manager_, context_pool_[job_id], data.data(), data.size(), HASH_FIRST);
    }


    void submit_update(std::size_t job_id, std::span<const std::byte> data) override
    {
        Expects(job_id < context_pool_.size());

        auto* ctx = context_pool_[job_id];
        while (hash_ctx_processing(ctx)) {
            Flush(context_manager_);
        }
        Submit(context_manager_, ctx, data.data(), data.size(), HASH_UPDATE);
    }

    void submit_last(std::size_t job_id, std::span<const std::byte> data) override
    {
        Expects(job_id < context_pool_.size());

        auto* ctx = context_pool_[job_id];
        while (hash_ctx_processing(ctx)) {
             Flush(context_manager_);
        }
        Submit(context_manager_, ctx, data.data(), data.size(), HASH_LAST);
    }

    std::size_t job_count() override
    {
        return context_pool_.size();
    }

    void finalize_to(std::size_t job_id, std::span<std::byte> buffer) override
    {
        Expects(job_id < context_pool_.size());

        while (!hash_ctx_complete(context_pool_[job_id])) {
            Flush(context_manager_);
        }
        // Intel ISA-L stores the hash as 32 bit words so we need to byteswap and copy per word
        for (std::size_t i = 0; i < N_WORDS; ++i) {
            auto big_endian_word = to_be32(hash_ctx_digest(context_pool_[job_id])[i]);
            std::memcpy(buffer.data()+ i * sizeof(big_endian_word),
                        &big_endian_word, sizeof(big_endian_word));
        }
    }

    void resize(std::size_t len) override
    {
        if (len == context_pool_.size())
            return;

        if (len > context_pool_.size()) {
            auto to_allocate = len - context_pool_.size();
            std::size_t idx;
            for (std::size_t i = 0; i < to_allocate; ++i) {
                allocate_hash_context(idx);
            }
        }

        if (len < context_pool_.size()) {
            auto to_deallocate = context_pool_.size() - len;
            std::size_t idx;
            for (std::size_t i = len; i < context_pool_.size(); ++i) {
                deallocate_hash_context(context_pool_[i]);
            }
            context_pool_.resize(len);
        }
    }

    void reset() override
    {
        for (auto ctx : context_pool_) {
            deallocate_hash_context(ctx);
        }
        context_pool_.clear();
    }

    ~multi_buffer_hasher_impl() override
    {
        for (auto ctx : context_pool_) {
            deallocate_hash_context(ctx);
        }
#ifdef USE_ALLIGED_MALLOC
        _aligned_free(context_manager_);
#else
        std::free(context_manager_);
#endif
    }

private:

    CTX* allocate_hash_context(std::size_t& index)
    {
        index = context_pool_.size();
        context_pool_.push_back(nullptr);

        constexpr std::size_t alignment = 64;
        constexpr std::size_t n = ((sizeof(CTX) + alignment -1) / alignment) * alignment;

#ifdef USE_ALLIGED_MALLOC
        CTX* hash_ctx = static_cast<CTX*>(_aligned_malloc(n, alignment));
#else
        CTX* hash_ctx = static_cast<CTX*>(std::aligned_alloc(alignment, n));
#endif
        if (hash_ctx == NULL)
            throw std::runtime_error("could not allocate hash context");

        hash_ctx_init(hash_ctx);
        hash_ctx->user_data = new user_data;
        static_cast<user_data*>(hash_ctx->user_data)->index = index;
        context_pool_.back() = hash_ctx;

        return hash_ctx;
    }

    void deallocate_hash_context(CTX* ctx)
    {
        delete static_cast<user_data*>(ctx->user_data);
#ifdef USE_ALLIGED_MALLOC
        _aligned_free(ctx);
#else
       std::free(ctx);
#endif
    }

    MGR* context_manager_ {};
    std::vector<CTX*> context_pool_ {};
};


extern template class multi_buffer_hasher_impl<
        MD5_HASH_CTX_MGR, MD5_HASH_CTX,
        &md5_ctx_mgr_init, &md5_ctx_mgr_submit, &md5_ctx_mgr_flush,
        MD5_DIGEST_NWORDS
>;

extern template class multi_buffer_hasher_impl<
        SHA1_HASH_CTX_MGR, SHA1_HASH_CTX,
        &sha1_ctx_mgr_init, &sha1_ctx_mgr_submit, &sha1_ctx_mgr_flush,
        SHA1_DIGEST_NWORDS
>;

extern template class multi_buffer_hasher_impl<
        SHA256_HASH_CTX_MGR, SHA256_HASH_CTX,
        &sha256_ctx_mgr_init, &sha256_ctx_mgr_submit, &sha256_ctx_mgr_flush,
        SHA256_DIGEST_NWORDS
>;

extern template class multi_buffer_hasher_impl<
        SHA512_HASH_CTX_MGR, SHA512_HASH_CTX,
        &sha512_ctx_mgr_init, &sha512_ctx_mgr_submit, &sha512_ctx_mgr_flush,
        SHA512_DIGEST_NWORDS
>;

using md5_multi_buffer_hasher = multi_buffer_hasher_impl<
        MD5_HASH_CTX_MGR, MD5_HASH_CTX,
        &md5_ctx_mgr_init, &md5_ctx_mgr_submit, &md5_ctx_mgr_flush,
        MD5_DIGEST_NWORDS
>;

using sha1_multi_buffer_hasher = multi_buffer_hasher_impl<
        SHA1_HASH_CTX_MGR, SHA1_HASH_CTX,
        &sha1_ctx_mgr_init, &sha1_ctx_mgr_submit, &sha1_ctx_mgr_flush,
        SHA1_DIGEST_NWORDS
>;

using sha256_multi_buffer_hasher = multi_buffer_hasher_impl<
        SHA256_HASH_CTX_MGR, SHA256_HASH_CTX,
        &sha256_ctx_mgr_init, &sha256_ctx_mgr_submit, &sha256_ctx_mgr_flush,
        SHA256_DIGEST_NWORDS
>;

using sha512_multi_buffer_hasher = multi_buffer_hasher_impl<
        SHA512_HASH_CTX_MGR, SHA512_HASH_CTX,
        &sha512_ctx_mgr_init, &sha512_ctx_mgr_submit, &sha512_ctx_mgr_flush,
        SHA512_DIGEST_NWORDS
>;

static const auto supported_hash_functions = std::unordered_set {
        dottorrent::hash_function::md5,
        dottorrent::hash_function::sha1,
        dottorrent::hash_function::sha256,
        dottorrent::hash_function::sha512,
};



}

#undef USE_ALLIGED_MALLOC
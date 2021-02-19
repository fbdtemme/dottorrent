#pragma once
#include <vector>
#include <span>
#include <string_view>
#include <cstring>

#include "dottorrent/hasher/multi_buffer_hasher.hpp"

#include <gsl-lite/gsl-lite.hpp>

//
#if __has_include("<isa-l_crypto/md5_mb.h>") \
        &&__has_include("<isa-l_crypto/sha1_mb.h>")  \
        &&__has_include("<isa-l_crypto/sha256_mb.h>")\
        &&__has_include("<isa-l_crypto/sha512_mb.h>")\
        &&__has_include("<isa-l_crypto/endian_helper.h>")
    #include <isa-l_crypto/md5_mb.h>
    #include <isa-l_crypto/sha1_mb.h>
    #include <isa-l_crypto/sha256_mb.h>
    #include <isa-l_crypto/sha512_mb.h>
    #include <isa-l_crypto/endian_helper.h>
#else
    #include <md5_mb.h>
    #include <sha1_mb.h>
    #include <sha256_mb.h>
    #include <sha512_mb.h>
    #include <endian_helper.h>
#endif


namespace isal {

enum class message_digest_algorithm
{
    sha1,
    sha256
};

template <typename MGR, typename CTX, auto Init, auto Submit, auto Flush, std::size_t N_WORDS>
class multi_buffer_hasher_impl : public multi_buffer_hasher
{
    using user_data = struct { std::size_t index; };

public:
    multi_buffer_hasher_impl()
    {
        context_manager_ = static_cast<MGR*>(aligned_alloc(64, sizeof(MGR)));
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
        auto* ctx = context_pool_[job_id];
        while (hash_ctx_processing(ctx)) {
            Flush(context_manager_);
        }
        Submit(context_manager_, ctx, data.data(), data.size(), HASH_UPDATE);
    }

    void submit_last(std::size_t job_id, std::span<const std::byte> data) override
    {
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
        free(context_manager_);
    }

private:

    CTX* allocate_hash_context(std::size_t& index)
    {
        index = context_pool_.size();
        context_pool_.push_back(nullptr);
        CTX* hash_ctx = static_cast<CTX*>(std::aligned_alloc(64, sizeof(CTX)));
        hash_ctx_init(hash_ctx);
        hash_ctx->user_data = new user_data;
        static_cast<user_data*>(hash_ctx->user_data)->index = index;
        context_pool_.back() = hash_ctx;

        return hash_ctx;
    }

    void deallocate_hash_context(CTX* ctx)
    {
        delete static_cast<user_data*>(ctx->user_data);
        free(ctx);
    }

    MGR* context_manager_ {};
    std::vector<CTX*> context_pool_ {};
};


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

}
#pragma once
#include <span>
#include <cstddef>
#include <unordered_set>
#include <memory>

#include "dottorrent/hasher/backends/isal.hpp"
#include "dottorrent/hasher/multi_buffer_hasher.hpp"

namespace dottorrent {

static const auto supported_hash_functions_isal = std::unordered_set {
        hash_function::sha1,
        hash_function::sha256,
};


class isal_multi_buffer_hasher : public multi_buffer_hasher
{
public:
    explicit isal_multi_buffer_hasher(hash_function f)
        : hasher_()
    {
        switch (f) {
        case hash_function::md5 : {
            hasher_ = std::make_unique<isal::md5_multi_buffer_hasher>();
            break;
        }
        case hash_function::sha1 : {
            hasher_ = std::make_unique<isal::sha1_multi_buffer_hasher>();
            break;
        }
        case hash_function::sha256 : {
            hasher_ = std::make_unique<isal::sha256_multi_buffer_hasher>();
            break;
        }
        case hash_function::sha512 : {
            hasher_ = std::make_unique<isal::sha512_multi_buffer_hasher>();
            break;
        }
        default:
            throw std::invalid_argument("No matching multi-buffer hasher for given algorithm");
        }
    }

    void submit(std::size_t job_id, std::span<const std::byte> data) override
    {
        hasher_->submit(job_id, data);
    }

    std::size_t submit(std::span<const std::byte> data) override
    {
        return hasher_->submit(data);
    }

    void submit_first(std::size_t job_idx, std::span<const std::byte> data) override
    {
        hasher_->submit_first(job_idx, data);
    }


    std::size_t submit_first(std::span<const std::byte> data) override
    {
        return hasher_->submit_first(data);
    }

    void submit_update(std::size_t job_id, std::span<const std::byte> data) override
    {
        hasher_->submit_update(job_id, data);
    }

    void submit_last(std::size_t job_id, std::span<const std::byte> data) override
    {
        hasher_->submit_last(job_id, data);
    }

    std::size_t job_count() override
    {
        return hasher_->job_count();
    }

    void finalize_to(std::size_t job_id, std::span<std::byte> buffer) override
    {
        hasher_->finalize_to(job_id, buffer);
    }

    void reset() override
    {
        hasher_->reset();
    }

    void resize(std::size_t len) override
    {
        hasher_->resize(len);
    }

    static const std::unordered_set<hash_function>& supported_algorithms() noexcept
    {
        return supported_hash_functions_isal;
    }

private:
    std::unique_ptr<multi_buffer_hasher> hasher_;
};

}
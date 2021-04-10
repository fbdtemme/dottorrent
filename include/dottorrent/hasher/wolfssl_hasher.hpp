#pragma once

#include "dottorrent/hash_function.hpp"
#include "dottorrent/hasher/single_buffer_hasher.hpp"
#include "dottorrent/hasher/backends/wolfssl.hpp"

namespace dottorrent {

class wolfssl_hasher : public single_buffer_hasher
{
public:
    explicit wolfssl_hasher(hash_function algorithm)
        : hasher_()
    {
        switch (algorithm) {
        case hash_function::md5 : {
            hasher_ = std::make_unique<wolfssl::md5_hasher>();
            break;
        }
        case hash_function::sha1 : {
            hasher_ = std::make_unique<wolfssl::sha1_hasher>();
            break;
        }
        case hash_function::sha256 : {
            hasher_ = std::make_unique<wolfssl::sha256_hasher>();
            break;
        }
        case hash_function::sha512 : {
            hasher_ = std::make_unique<wolfssl::sha512_hasher>();
            break;
        }
        default:
            throw std::invalid_argument("No matching hasher for given algorithm");
        }
    }

    void update(std::span<const std::byte> data) override
    {
        hasher_->update(data);
    }

    void finalize_to(std::span<std::byte> out) override
    {
        hasher_->finalize_to(out);
    }

    static const std::unordered_set<hash_function>& supported_algorithms() noexcept
    {
        return wolfssl::supported_hash_functions;
    }

private:
    std::unique_ptr<single_buffer_hasher> hasher_;
};

}
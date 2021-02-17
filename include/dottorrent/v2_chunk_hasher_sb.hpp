#pragma once
#include <span>
#include <vector>
#include <mutex>
#include <atomic>

#include <gsl-lite/gsl-lite.hpp>

#include "dottorrent/merkle_tree.hpp"
#include "dottorrent/chunk_hasher_multi_buffer.hpp"
#include "dottorrent/v2_chunk_hasher_mixin.hpp"

namespace dottorrent {

class v2_chunk_hasher_sb : public chunk_hasher_single_buffer, public v2_chunk_hasher_mixin
{
public:
    using base_type = chunk_hasher_single_buffer;

    explicit v2_chunk_hasher_sb(file_storage& storage, std::size_t capacity, bool v1_compatible = false, std::size_t thread_count = 1);

protected:
    void hash_chunk(std::vector<std::unique_ptr<single_buffer_hasher>>& hashers, const data_chunk& chunk) override;

    void hash_chunk(single_buffer_hasher& sha256_hasher, single_buffer_hasher& sha1_hasher, const data_chunk& chunk);
};

} // namespace dottorrent

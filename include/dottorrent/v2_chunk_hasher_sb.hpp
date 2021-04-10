#pragma once
#include <span>
#include <vector>
#include <mutex>
#include <atomic>

#include <gsl-lite/gsl-lite.hpp>

#include "dottorrent/chunk_hasher_single_buffer.hpp"

namespace dottorrent {

class v2_chunk_hasher_sb : public chunk_hasher_single_buffer
{
public:
    using base_type = chunk_hasher_single_buffer;

    explicit v2_chunk_hasher_sb(file_storage& storage, std::size_t capacity, bool v1_compatible = false, std::size_t thread_count = 1);

protected:
    void initialize_v1_offsets(const file_storage& storage);

    void hash_chunk(std::vector<std::unique_ptr<single_buffer_hasher>>& hashers, const data_chunk& chunk) override;

    void hash_chunk(single_buffer_hasher& sha256_hasher, single_buffer_hasher& sha1_hasher, const data_chunk& chunk);

    void process_piece_hash(std::size_t piece_idx, std::size_t file_index, const sha1_hash& piece_hash);

    void process_piece_hash(std::size_t leaf_index, std::size_t file_index, const sha256_hash& piece_hash);

private:
    bool add_v1_compatibility_ = false;
    // for each file the v1 piece index the file starts at.
    std::vector<std::size_t> v1_piece_offsets_ {};
    std::vector<std::byte> padding_ {};
    std::mutex padding_mutex_ {};
};

} // namespace dottorrent

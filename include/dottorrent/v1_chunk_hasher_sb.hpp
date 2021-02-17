#pragma once
#include <span>

#include "dottorrent/file_storage.hpp"
#include "dottorrent/chunk_hasher_single_buffer.hpp"
#include "dottorrent/hash.hpp"
#include "dottorrent/data_chunk.hpp"

namespace dottorrent {

class v1_chunk_hasher_sb : public chunk_hasher_single_buffer
{
public:
    using base_type = chunk_hasher_single_buffer;
    using hash_type = sha1_hash;

    explicit v1_chunk_hasher_sb(file_storage& storage, std::size_t capacity, std::size_t thread_count = 1);

protected:
    void hash_chunk(std::vector<std::unique_ptr<single_buffer_hasher>>& hashers, const data_chunk& chunk) override;

    void hash_chunk(single_buffer_hasher& hasher, const data_chunk& chunk);

    void process_piece_hash(std::size_t piece_idx, std::size_t file_idx, const sha1_hash& piece_hash);
};

}
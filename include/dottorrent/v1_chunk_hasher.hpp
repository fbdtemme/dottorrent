#pragma once
#include <span>

#include "dottorrent/file_storage.hpp"
#include "dottorrent/chunk_hasher.hpp"
#include "dottorrent/hash.hpp"
#include "dottorrent/data_chunk.hpp"

namespace dottorrent {

class v1_chunk_hasher : public chunk_hasher
{
public:
    using hash_type = sha1_hash;

    v1_chunk_hasher(file_storage& storage, std::size_t thread_count);

protected:

    void hash_chunk(hasher& hasher, const data_chunk& chunk) override;

    virtual void process_piece_hash(std::size_t piece_idx,
                                    std::size_t file_idx,
                                    const sha1_hash& piece_hash);
};

}
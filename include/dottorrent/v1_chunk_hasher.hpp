#pragma once
#include <span>

#include "dottorrent/file_storage.hpp"
#include "dottorrent/chunk_hasher.hpp"
#include "dottorrent/hash.hpp"
#include "dottorrent/data_chunk.hpp"

namespace dottorrent {

class v1_chunk_hasher : public chunk_hasher<sha1_hasher>
{
public:
    using base_type = chunk_hasher<sha1_hasher>;
    using base_type::base_type;

protected:
    void hash_chunk(hasher_type& hasher, const data_chunk& chunk) override;

    virtual void process_piece_hash(std::size_t piece_idx,
                                    std::size_t file_idx,
                                    const hash_type& piece_hash);
};

}
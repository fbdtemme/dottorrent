#pragma once

#include "dottorrent/v1_chunk_hasher.hpp"

namespace dottorrent
{

class v1_chunk_verifier : public v1_chunk_hasher
{
public:
    using base = v1_chunk_hasher;

    v1_chunk_verifier(file_storage& storage, std::size_t thread_count);

    const std::vector<std::uint8_t>& result();

protected:
    void process_piece_hash(std::size_t piece_idx, std::size_t file_idx, const sha1_hash& piece_hash) override;

private:
    std::vector<std::uint8_t> piece_map_;
};

}
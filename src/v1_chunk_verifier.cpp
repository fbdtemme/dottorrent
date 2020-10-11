#include "dottorrent/v1_chunk_verifier.hpp"

namespace dottorrent {

v1_chunk_verifier::v1_chunk_verifier(file_storage& storage, std::size_t thread_count)
        : base(storage, thread_count)
        , piece_map_(storage.pieces().size())
{}

void v1_chunk_verifier::process_piece_hash(
        std::size_t piece_idx,
        std::size_t file_idx,
        const sha1_hash& piece_hash)
{
    file_storage& storage = storage_;
    const auto& real_hash = storage.get_piece_hash(piece_idx);
    piece_map_[piece_idx] = (real_hash == piece_hash);
    pieces_done_.fetch_add(1, std::memory_order_relaxed);
}

const std::vector<std::uint8_t>& v1_chunk_verifier::result() {
    return piece_map_;
}
}
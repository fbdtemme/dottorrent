#include "dottorrent/v1_chunk_hasher.hpp"

namespace dottorrent
{

void v1_chunk_hasher::hash_chunk(sha1_hasher& hasher, const data_chunk& chunk)
{
    file_storage& storage = storage_;
    std::size_t piece_size = storage.piece_size();

    // Piece without any data indicate a missing file.
    // We do not upgrade bytes hashed but mark the piece as done.
    if (chunk.data == nullptr) {
        pieces_done_.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    const auto pieces_in_chunk = (chunk.data->size() + piece_size-1) / piece_size;
    auto data = std::span(*chunk.data);

    if (pieces_in_chunk == 1) {
        hasher.update(data);
        process_piece_hash(chunk.piece_index, chunk.file_index, hasher.finalize());
    }
    else {
        std::size_t piece_offset = 0;
        for (; piece_offset < pieces_in_chunk - 1; ++piece_offset) {
            hasher.update(data.subspan(piece_size * piece_offset, piece_size));
            process_piece_hash(chunk.piece_index + piece_offset, chunk.file_index, hasher.finalize());
        }

        // last piece of a chunk can be smaller than the full piece_size
        hasher.update(data.subspan(piece_offset * piece_size));
        process_piece_hash(chunk.piece_index + piece_offset, chunk.file_index, hasher.finalize());
    }

    bytes_hashed_.fetch_add(chunk.data->size(), std::memory_order_relaxed);
}

void v1_chunk_hasher::process_piece_hash(
        std::size_t piece_idx,
        std::size_t file_idx,
        const sha1_hash& piece_hash)
{
    file_storage& storage = storage_;
    storage.set_piece_hash(piece_idx, piece_hash);
    pieces_done_.fetch_add(1, std::memory_order_relaxed);
}
}
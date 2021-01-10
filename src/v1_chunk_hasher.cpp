#include "dottorrent/v1_chunk_hasher.hpp"

namespace dottorrent
{
v1_chunk_hasher::v1_chunk_hasher(file_storage& storage, std::size_t thread_count)
        : chunk_hasher(storage, hash_function::sha1, thread_count)
{}

void v1_chunk_hasher::hash_chunk(hasher& hasher, const data_chunk& chunk)
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

    Expects(pieces_in_chunk >= 1);
    sha1_hash piece_hash{};

    std::size_t piece_in_block_idx = 0;
    for (; piece_in_block_idx < pieces_in_chunk - 1; ++piece_in_block_idx) {
        hasher.update(data.subspan(piece_size * piece_in_block_idx, piece_size));
        hasher.finalize_to(piece_hash);
        process_piece_hash(chunk.piece_index + piece_in_block_idx, chunk.file_index, piece_hash);
    }

    // last piece of a chunk can be smaller than the full piece_size
    hasher.update(data.subspan(piece_in_block_idx * piece_size));
    hasher.finalize_to(piece_hash);
    process_piece_hash(chunk.piece_index + piece_in_block_idx, chunk.file_index, piece_hash);

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
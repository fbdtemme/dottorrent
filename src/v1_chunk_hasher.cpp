#include "dottorrent/v1_chunk_hasher.hpp"

namespace dottorrent
{
v1_chunk_hasher::v1_chunk_hasher(file_storage& storage, std::size_t thread_count)
        : chunk_hasher(storage, thread_count)
{
    hasher_ = make_hasher(hash_function::sha1);
}

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

    sha1_hash piece_hash{};

    if (pieces_in_chunk == 1) {
        hasher.update(data);
        hasher.finalize_to(piece_hash);
        process_piece_hash(chunk.piece_index, chunk.file_index, piece_hash);
    }
    else {
        std::size_t piece_offset = 0;
        for (; piece_offset < pieces_in_chunk - 1; ++piece_offset) {
            hasher.update(data.subspan(piece_size * piece_offset, piece_size));
            hasher.finalize_to(piece_hash);
            process_piece_hash(chunk.piece_index + piece_offset, chunk.file_index, piece_hash);
        }

        // last piece of a chunk can be smaller than the full piece_size
        hasher.update(data.subspan(piece_offset * piece_size));
        hasher.finalize_to(piece_hash);
        process_piece_hash(chunk.piece_index + piece_offset, chunk.file_index, piece_hash);
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
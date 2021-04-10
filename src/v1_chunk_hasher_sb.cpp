#include "dottorrent/v1_chunk_hasher_sb.hpp"

namespace dottorrent
{
v1_chunk_hasher_sb::v1_chunk_hasher_sb(file_storage& storage, std::size_t capacity, std::size_t thread_count)
        : base_type(storage, {hash_function::sha1}, capacity, thread_count)
{}

void v1_chunk_hasher_sb::hash_chunk(std::vector<std::unique_ptr<single_buffer_hasher>>& hashers, const data_chunk& chunk)
{
    hash_chunk(*hashers.front(), chunk);
}

void v1_chunk_hasher_sb::hash_chunk(single_buffer_hasher& hasher, const data_chunk& chunk)
{
    file_storage& storage = storage_;
    std::size_t piece_size = storage.piece_size();

    // Piece without any data indicate a missing file.
    // We do not upgrade bytes hashed but mark the piece as done.
    if (chunk.data == nullptr) {
        bytes_done_.fetch_add(piece_size, std::memory_order_relaxed);
        return;
    }

    const auto pieces_in_chunk = (chunk.data->size() + piece_size - 1) / piece_size;
    auto data = std::span(*chunk.data);

    Expects(pieces_in_chunk >= 1);
    sha1_hash piece_hash{};

    std::size_t piece_in_block_idx = 0;
    for (; piece_in_block_idx < pieces_in_chunk - 1; ++piece_in_block_idx) {
        hasher.update(data.subspan(piece_size * piece_in_block_idx, piece_size));
        hasher.finalize_to(piece_hash);
        process_piece_hash(chunk.piece_index + piece_in_block_idx, chunk.file_index, piece_hash);
        bytes_done_.fetch_add(piece_size, std::memory_order_relaxed);
    }

    // last piece of a chunk can be smaller than the full piece_size
    auto final_piece = data.subspan(piece_in_block_idx * piece_size);
    hasher.update(final_piece);
    hasher.finalize_to(piece_hash);
    process_piece_hash(chunk.piece_index + piece_in_block_idx, chunk.file_index, piece_hash);
    bytes_done_.fetch_add(final_piece.size(), std::memory_order_relaxed);

    bytes_hashed_.fetch_add(chunk.data->size(), std::memory_order_relaxed);
}

void v1_chunk_hasher_sb::process_piece_hash(
        std::size_t piece_idx,
        std::size_t file_idx,
        const sha1_hash& piece_hash)
{
    Expects(v1_hashed_piece_queue_);
    v1_hashed_piece_queue_->push(v1_hashed_piece{.hash=piece_hash, .index=piece_idx});
}

}
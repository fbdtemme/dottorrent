#include <gsl-lite/gsl-lite.hpp>
#include "dottorrent/hashed_piece.hpp"
#include "dottorrent/v2_chunk_hasher_sb.hpp"

namespace dottorrent {

v2_chunk_hasher_sb::v2_chunk_hasher_sb(file_storage& storage, std::size_t capacity, bool v1_compatible, std::size_t thread_count)
        : chunk_hasher_single_buffer(storage, {hash_function::sha1, hash_function::sha256}, capacity, thread_count)
        , add_v1_compatibility_(v1_compatible)
{
    initialize_v1_offsets(storage);
}

void v2_chunk_hasher_sb::initialize_v1_offsets(const file_storage& storage)
{
    auto piece_size = storage.piece_size();
    v1_piece_offsets_.push_back(0);

    for (const auto& entry : storage) {
        auto block_count = (entry.file_size()+v2_block_size-1)/v2_block_size;
        if (entry.is_padding_file()) {
            v1_piece_offsets_.push_back(v1_piece_offsets_.back());
        }
        else {
            v1_piece_offsets_.push_back(
                    v1_piece_offsets_.back()+(entry.file_size() + piece_size - 1) / piece_size);
        }
    }
}

void v2_chunk_hasher_sb::hash_chunk(std::vector<std::unique_ptr<single_buffer_hasher>>& hashers, const data_chunk& chunk)
{
    hash_chunk(*hashers.back(), *hashers.front(), chunk);
}

void v2_chunk_hasher_sb::hash_chunk(single_buffer_hasher& sha256_hasher, single_buffer_hasher& sha1_hasher,
        const data_chunk& chunk)
{
    file_storage& storage = storage_.get();
    Expects(chunk.file_index < storage.file_count());

    // Piece without any data indicate a missing file.
    // We do not upgrade bytes hashed but mark the whole file as done.
    if (chunk.data == nullptr) {
        auto file_size = storage[chunk.file_index].file_size();
        bytes_done_.fetch_add(file_size, std::memory_order_relaxed);
        return;
    }

    file_entry& entry = storage[chunk.file_index];
    const auto piece_size = storage.piece_size();
    const auto pieces_in_chunk = detail::div_ceil(chunk.data->size(), piece_size);
    // number of 16 KiB blocks in a chunk
    const auto blocks_in_chunk = detail::div_ceil(chunk.data->size(), v2_block_size);
    // index of first 16 KiB block in the per file merkle tree
    const auto index_offset = chunk.piece_index * piece_size / v2_block_size;
    const auto data = std::span(*chunk.data);

    sha256_hash leaf{};

    std::size_t i = 0;
    for (; blocks_in_chunk != 0 && i < blocks_in_chunk-1; ++i) {
        sha256_hasher.update(data.subspan(i*v2_block_size, v2_block_size));
        sha256_hasher.finalize_to(leaf);
        process_piece_hash(index_offset+i, chunk.file_index, leaf);
        bytes_hashed_.fetch_add(v2_block_size);
    }

    // last block can be smaller then tlockhe block size!
    auto final_block = data.subspan(i*v2_block_size);
    sha256_hasher.update(final_block);
    sha256_hasher.finalize_to(leaf);
    bytes_hashed_.fetch_add(final_block.size());
    process_piece_hash(index_offset+i, chunk.file_index, leaf);
//
//    // Update per file progress and check if this thread did just finish the last chunk of
//    // this file. Make sure to propagate memory effects so set_piece_layers sees all
//    // leaf nodes of the merkle tree.
//    auto tmp = file_progress.fetch_add(chunk.data->size(), std::memory_order_acq_rel);
//    if (tmp == (entry.file_size()-chunk.data->size())) [[unlikely]] {
//        set_piece_layers_and_root(storage, sha256_hasher, chunk.file_index);
//    }

    if (add_v1_compatibility_) {
        // v1 compatibility data
        sha1_hash piece_hash{};

        bool needs_padding = chunk.data->size() % piece_size != 0;
        std::size_t pieces_to_process = 0;

        // process the complete pieces of the chunk
        if (!needs_padding) {
            pieces_to_process = pieces_in_chunk;
        }
        else {
            // last piece needs padding or is the last piece of the file
            pieces_to_process = pieces_in_chunk != 0 ? pieces_in_chunk-1 : 0;
        }

        std::size_t piece_in_chunk_index = 0;
        for (; piece_in_chunk_index < pieces_to_process; ++piece_in_chunk_index) {
            sha1_hasher.update(data.subspan(piece_size*piece_in_chunk_index, piece_size));
            sha1_hasher.finalize_to(piece_hash);
            process_piece_hash(chunk.piece_index+piece_in_chunk_index, chunk.file_index, piece_hash);
            bytes_hashed_.fetch_add(piece_size, std::memory_order_relaxed);
        }

        // we have an incomplete final piece so we have the last piece of a file.
        // we need to pad with zeros in case it is not the last file in the torrent
        if (needs_padding) {
            if (chunk.file_index+1 < storage.file_count()-1) {
                const auto& entry = storage.at(chunk.file_index+1);
                Expects(entry.is_padding_file());
                // add the final partial piece and pad the rest of the piece
                auto final_piece = data.subspan(piece_in_chunk_index*piece_size);
                sha1_hasher.update(final_piece);
                std::size_t padding_size = piece_size-final_piece.size();

                {
                    std::unique_lock lck{padding_mutex_};
                    padding_.resize(padding_size, std::byte{0});
                    sha1_hasher.update(padding_);
                }

                sha1_hasher.finalize_to(piece_hash);
                process_piece_hash(chunk.piece_index+piece_in_chunk_index, chunk.file_index, piece_hash);
                bytes_hashed_.fetch_add(final_piece.size()+padding_size, std::memory_order::relaxed);
            }
            else {
                auto final_piece = data.subspan(piece_in_chunk_index*piece_size);
                sha1_hasher.update(final_piece);
                sha1_hasher.finalize_to(piece_hash);
                process_piece_hash(chunk.piece_index+piece_in_chunk_index, chunk.file_index, piece_hash);
                bytes_hashed_.fetch_add(final_piece.size(), std::memory_order::relaxed);
            }
        }
    }
    bytes_done_.fetch_add(chunk.data->size(), std::memory_order_relaxed);
}

void v2_chunk_hasher_sb::process_piece_hash(std::size_t piece_idx, std::size_t file_index, const sha1_hash& piece_hash)
{
    Expects(v1_hashed_piece_queue_);
    auto global_piece_index = v1_piece_offsets_[file_index] + piece_idx;
    v1_hashed_piece_queue_->push(v1_hashed_piece{.hash=piece_hash, .index=global_piece_index});
}

void v2_chunk_hasher_sb::process_piece_hash(std::size_t leaf_index, std::size_t file_index, const sha256_hash& piece_hash)
{
    Expects(v2_hashed_piece_queue_);
    v2_hashed_piece_queue_->push(v2_hashed_piece{.hash=piece_hash, .file_index=file_index, .leaf_index=leaf_index});
}

} // namepspace dottorrent
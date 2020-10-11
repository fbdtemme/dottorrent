//
// Created by fbdtemme on 9/22/20.
//
#include "dottorrent/v2_chunk_hasher.hpp"

namespace dottorrent {

v2_chunk_hasher::v2_chunk_hasher(file_storage& storage, std::size_t thread_count)
        : base_type(storage, thread_count)
        , piece_size_(storage.piece_size())
        , file_bytes_hashed_()
{
    // SHA265 hash of 16 KiB of zero bytes.
    static constexpr auto default_hash = make_hash_from_hex<sha256_hash>(
            "4fe7b59af6de3b665b67788cc2f99892ab827efae3a467342b3bb4e3bc8e5bfe");
    const auto& st = storage_.get();
    std::size_t file_count = 0;

    for (const auto& entry : storage) {
        ++file_count;

        if (entry.is_padding_file()) {
            merkle_trees_.emplace_back();
        } else {
            auto block_count = (entry.file_size() + v2_block_size -1) / v2_block_size;
            merkle_trees_.emplace_back(block_count, default_hash);
        }
    }
    file_bytes_hashed_ = decltype(file_bytes_hashed_)(file_count);
    hasher_ = make_hasher(hash_function::sha256);
}

void v2_chunk_hasher::hash_chunk(hasher& hasher, const data_chunk& chunk) {
    file_storage& storage = storage_.get();

    Expects(chunk.file_index < storage.file_count());

    file_entry& entry = storage[chunk.file_index];
    auto& tree = merkle_trees_[chunk.file_index];
    auto& file_progress = file_bytes_hashed_[chunk.file_index];
    // number of 16 KiB blocks in a chunk
    const auto blocks_in_chunk = (chunk.data->size() + v2_block_size -1) / v2_block_size;
    // index of 16 KiB chunks in the per file merkle tree
    const auto index_offset = chunk.piece_index * piece_size_ / v2_block_size;
    const auto data = std::span(*chunk.data);

    // Piece without any data indicate a missing file.
    // We do not upgrade bytes hashed but mark the piece as done.
    if (chunk.data == nullptr) {
        pieces_done_.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    std::size_t i = 0;
    for ( ; i < blocks_in_chunk-1; ++i) {
        sha256_hash leaf {};
        hasher.update(data.subspan(i * v2_block_size, v2_block_size));
        hasher.finalize_to(leaf);
        tree.set_leaf(index_offset + i, leaf);

        pieces_done_.fetch_add(1, std::memory_order_relaxed);
    }

    // last block can be smaller then the block size!
    sha256_hash leaf {};
    hasher.update(data.subspan(i * v2_block_size));
    hasher.finalize_to(leaf);
    tree.set_leaf(index_offset + i, leaf);
    bytes_hashed_.fetch_add(chunk.data->size(), std::memory_order_relaxed);

    // Update per file progress and check if this thread did just finish the last chunk of
    // this file. Make sure to propagate memory effects so set_piece_layers sees all
    // leaf nodes of the merkle tree.
    auto tmp = file_progress.fetch_add(chunk.data->size(), std::memory_order_acq_rel);
    if (tmp == (entry.file_size() - chunk.data->size())) [[unlikely]] {
        set_piece_layers_and_root(hasher, chunk.file_index);
    }
}

void v2_chunk_hasher::set_piece_layers_and_root(hasher& hasher, std::size_t file_index) {
    Ensures(piece_size_ >= v2_block_size);
    Ensures(piece_size_ % v2_block_size == 0);

    file_storage& storage = storage_;
    auto& tree = merkle_trees_[file_index];

    // complete the merkle tree
    tree.update(hasher);

    // the depth in the tree of hashes covering blocks of size `piece_size`
    auto layer_offset = detail::log2_floor(piece_size_) - detail::log2_floor(v2_block_size);

    // set root hash
    file_entry& entry = storage.at(file_index);
    entry.set_pieces_root(tree.root());

    // files smaller then piece size have empty piece_layers
    if (layer_offset >= tree.tree_height()) {
        entry.set_piece_layer({});
        return;
    }

    std::size_t layer_depth = tree.tree_height() - layer_offset;

    // leaf nodes necessary to balance the tree are not included in the piece layers
    auto layer_view = tree.get_layer(layer_depth);
    auto layer_data_nodes_size = (entry.file_size() + piece_size_ - 1 ) / piece_size_;

    if (layer_view.size() != layer_data_nodes_size) {
        layer_view = layer_view.subspan(0, layer_data_nodes_size);
    }

    entry.set_piece_layer(layer_view);
}

}
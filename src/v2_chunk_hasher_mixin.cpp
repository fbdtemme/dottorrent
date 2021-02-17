#include "dottorrent/v2_chunk_hasher_mixin.hpp"

namespace dottorrent {

void v2_chunk_hasher_mixin::initialize_trees_and_v1_offsets(const file_storage& storage)
{
    auto piece_size = storage.piece_size();
    // SHA265 hash of 16 KiB of zero bytes.
    std::size_t file_count = 0;

    v1_piece_offsets_.push_back(0);

    for (const auto& entry : storage) {
        ++ file_count;
        auto block_count = (entry.file_size()+v2_block_size-1)/v2_block_size;
        if (entry.is_padding_file()) {
            v1_piece_offsets_.push_back(v1_piece_offsets_.back());
            // add en empty merkly tree to make sure file_indices match merkle tree indices.
            merkle_trees_.emplace_back();
        }
        else {
            merkle_trees_.emplace_back(block_count);
            v1_piece_offsets_.push_back(
                    v1_piece_offsets_.back()+(entry.file_size()+piece_size-1)/piece_size);
        }
    }
    file_bytes_hashed_ = decltype(file_bytes_hashed_)(file_count);
}

void v2_chunk_hasher_mixin::process_piece_hash(file_storage& storage, std::size_t piece_idx,
                                               std::size_t file_index, const sha1_hash& piece_hash)
{
    auto global_piece_index = v1_piece_offsets_[file_index] + piece_idx;
    storage.set_piece_hash(global_piece_index, piece_hash);
}

template <typename HasherType>
void v2_chunk_hasher_mixin::set_piece_layers_and_root(file_storage& storage, HasherType& hasher, std::size_t file_index) {
    std::size_t piece_size = storage.piece_size();

    Expects(piece_size >= v2_block_size);
    Expects(piece_size % v2_block_size == 0);

    auto& tree = merkle_trees_[file_index];

    // complete the merkle tree
    tree.update(hasher);

    // the depth in the tree of hashes covering blocks of size `piece_size`
    auto layer_offset = detail::log2_floor(piece_size) - detail::log2_floor(v2_block_size);

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
    auto layer_data_nodes_size = (entry.file_size() + piece_size - 1 ) / piece_size;

    if (layer_view.size() != layer_data_nodes_size) {
        layer_view = layer_view.subspan(0, layer_data_nodes_size);
    }

    entry.set_piece_layer(layer_view);
}

template void v2_chunk_hasher_mixin::set_piece_layers_and_root<>(file_storage&, single_buffer_hasher&, std::size_t);


#if defined (DOTTORRENT_USE_ISAL)
template void v2_chunk_hasher_mixin::set_piece_layers_and_root<>(file_storage&, multi_buffer_hasher&, std::size_t);
#endif

} // namespace dottorrent
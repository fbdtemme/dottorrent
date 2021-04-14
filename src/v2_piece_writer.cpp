#include "dottorrent/v2_piece_writer.hpp"


namespace dottorrent {

v2_piece_writer::v2_piece_writer(file_storage& storage, std::size_t capacity, bool v1_compatible,
        std::size_t max_concurrency)
        : storage_(storage)
        , add_v1_compatibility_(v1_compatible)
        , v1_processor_(capacity)
        , v2_processor_(capacity)
{
    initialize_trees(storage);
    if (add_v1_compatibility_) {
        v1_processor_.set_max_concurrency(max_concurrency);
        v1_processor_.set_work_function([this](const v1_hashed_piece& p) { this->set_finished_piece(p); });
    }
    v2_processor_.set_max_concurrency(max_concurrency);
    v2_processor_.set_work_function([this](const v2_hashed_piece& p) { this->set_finished_piece(p); });
}

void v2_piece_writer::start() {
    if (add_v1_compatibility_) v1_processor_.start();
    v2_processor_.start();
}

void v2_piece_writer::request_cancellation() {
    if (add_v1_compatibility_) v1_processor_.request_cancellation();
    v2_processor_.request_cancellation();
}

void v2_piece_writer::request_stop() {
    if (add_v1_compatibility_) v1_processor_.request_stop();
    v2_processor_.request_stop();
}

void v2_piece_writer::wait() {
    if (add_v1_compatibility_) v1_processor_.wait();
    v2_processor_.wait();
}

bool v2_piece_writer::running() const noexcept
{
    auto res = v2_processor_.running();
    if (add_v1_compatibility_) {
        res = res && v1_processor_.running();
    }
    return res;
};

bool v2_piece_writer::started() const noexcept
{
    auto res = v2_processor_.started();
    if (add_v1_compatibility_) {
        res = res && v1_processor_.started();
    }
    return res;
}

bool v2_piece_writer::cancelled() const noexcept
{
    auto res = v2_processor_.cancelled();
    if (add_v1_compatibility_) {
        res = res && v1_processor_.cancelled();
    }
    return res;
}

bool v2_piece_writer::done() const noexcept
{
    auto res = v2_processor_.done();
    if (add_v1_compatibility_) {
        res = res && v1_processor_.done();
    }
    return res;
}

std::shared_ptr<v2_piece_writer::v1_piece_queue_type> v2_piece_writer::get_v1_queue() {
    if (add_v1_compatibility_) return v1_processor_.get_queue();
    return nullptr;
}

std::shared_ptr<v2_piece_writer::v2_piece_queue_type> v2_piece_writer::get_v2_queue() {
    return v2_processor_.get_queue();
}

void v2_piece_writer::initialize_trees(const file_storage& storage) {
    auto piece_size = storage.piece_size();
    // SHA265 hash of 16 KiB of zero bytes.
    std::size_t file_count = 0;

    for (const auto& entry : storage) {
        ++ file_count;
        auto block_count = (entry.file_size() + v2_block_size - 1 ) / v2_block_size;
        if (entry.is_padding_file()) {
            // add en empty merkly tree to make sure file_indices match merkle tree indices.
            merkle_trees_.emplace_back();
        }
        else {
            merkle_trees_.emplace_back(block_count);
        }
    }
    file_blocks_hashed_ = decltype(file_blocks_hashed_)(file_count);
}

template<typename HasherType>
void v2_piece_writer::set_piece_layers_and_root(file_storage& storage, HasherType& hasher, std::size_t file_index)
{
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

void v2_piece_writer::set_finished_piece(const v1_hashed_piece& finished_piece) {
    file_storage& storage = storage_.get();
    storage.set_piece_hash(finished_piece.index, finished_piece.hash);
}

void v2_piece_writer::set_finished_piece(const v2_hashed_piece& finished_piece) {
#ifdef DOTTORRENT_USE_ISAL
    static thread_local std::unique_ptr<multi_buffer_hasher> hasher = make_multi_buffer_hasher(hash_function::sha256);
#else
    static thread_local std::unique_ptr<single_buffer_hasher> hasher = make_hasher(hash_function::sha256);
#endif

    file_storage& storage = storage_;
    file_entry& entry = storage[finished_piece.file_index];
    auto& tree = merkle_trees_[finished_piece.file_index];
    auto& file_progress = file_blocks_hashed_[finished_piece.file_index];

    tree.set_leaf(finished_piece.leaf_index, finished_piece.hash);

    // If this was the last leaf node we can complete this file by setting the piece_layers and root.
    auto num_blocks_in_file = detail::div_ceil(entry.file_size(), 16_KiB);

    auto tmp = file_progress.fetch_add(1, std::memory_order_acq_rel);
    if (num_blocks_in_file <= (tmp + 1)) [[unlikely]] {
        set_piece_layers_and_root(storage, *hasher, finished_piece.file_index);
    }
}

}

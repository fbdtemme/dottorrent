#include "dottorrent/v2_piece_verifier.hpp"

namespace dottorrent {

v2_piece_verifier::v2_piece_verifier(dottorrent::file_storage& storage, std::size_t capacity,
        std::size_t max_concurrency)
        : storage_(storage)
        , processor_(capacity)
        , merkle_trees_()
        , piece_map_()
        , file_offsets_()
{
    initialize_offsets_and_trees();

    processor_.set_max_concurrency(max_concurrency);
    processor_.set_work_function([this](const v2_hashed_piece& p) { this->verify_finished_piece(p); });
}

void v2_piece_verifier::start()
{
    processor_.start();
}

void v2_piece_verifier::request_stop()
{
    processor_.request_stop();
}

void v2_piece_verifier::request_cancellation()
{
    processor_.request_cancellation();
}

void v2_piece_verifier::wait() {
    processor_.wait();
}

std::shared_ptr<v2_piece_verifier::v1_piece_queue_type> v2_piece_verifier::get_v1_queue() {
    return nullptr;
}

std::shared_ptr<v2_piece_verifier::v2_piece_queue_type> v2_piece_verifier::get_v2_queue() {
    return processor_.get_queue();
}

const std::vector<std::uint8_t>& v2_piece_verifier::result() const noexcept {
    return piece_map_;
}

double v2_piece_verifier::percentage(std::size_t file_index) const noexcept {
    Expects(file_index < file_offsets_.size());
    constexpr std::size_t max_size_t = std::numeric_limits<std::size_t>::max();

    const file_storage& storage = storage_;
    const auto& entry = storage[file_index];

    if (entry.is_padding_file() || entry.file_size() == 0)
        return 100;

    std::size_t number_of_pieces = 1;
    if (auto s = entry.piece_layer().size(); s > 0) {
        number_of_pieces = s;
    }

    auto first_piece_map_index = std::transform_reduce(
            storage.begin(), std::next(storage.begin(), file_index),
            0ul, std::plus<>{},
            [](const file_entry& e) {
                if (e.is_padding_file()) return std::size_t(0);
                return std::max(std::size_t(1), e.piece_layer().size());
            });

    auto n_complete_pieces = std::count(
            std::next(piece_map_.begin(), first_piece_map_index),
            std::next(piece_map_.begin(), first_piece_map_index+number_of_pieces), 1);

    return double(n_complete_pieces) / double(number_of_pieces) * 100;
}

void v2_piece_verifier::initialize_offsets_and_trees() {
    // SHA265 hash of 16 KiB of zero bytes.
    file_storage& storage = storage_;
    std::size_t file_count = 0;
    std::size_t piece_size = storage.piece_size();

    Expects(piece_size >= v2_block_size);
    Expects(piece_size % v2_block_size == 0);

    file_offsets_.push_back(0);

    for (const auto& entry : storage) {
        ++file_count;
        auto block_count = (entry.file_size() + v2_block_size -1) / v2_block_size;
        if (entry.is_padding_file()) {
            // add en empty merkly tree to make sure file_indices match merkle tree indices.
            merkle_trees_.emplace_back();
            auto offset = file_offsets_.back();
            file_offsets_.back() = std::numeric_limits<std::size_t>::max();
            file_offsets_.push_back(offset);
        }
        else {
            auto& m = merkle_trees_.emplace_back(block_count);
            auto offset = std::max(std::size_t(1), entry.piece_layer().size());

            // get the offset of the last non-padding file
            std::size_t previous_offset = 0;
            // check if last offset is a padding file
            if (file_offsets_.size() == 0) {
                previous_offset = 0;
            }
            else if (file_offsets_.back() == std::numeric_limits<std::size_t>::max()) {
                previous_offset = *std::prev(file_offsets_.end(), 2);
            } else {
                previous_offset = *std::prev(file_offsets_.end(), 1);
            }
            file_offsets_.push_back(previous_offset + offset);
        }
    }

    file_blocks_hashed_ = decltype(file_blocks_hashed_)(file_count);

    auto piece_map_size = 0;
    for (const auto& entry : storage) {
        if (entry.is_padding_file())
            continue;

        auto s = entry.piece_layer().size();
        if (s != 0) {
            piece_map_size += s;
        }
        else {
            // we have a file smaller then the piece size with only a root hash.
            // or a padding file
            piece_map_size += 1;
        }
    }
    piece_map_.resize(piece_map_size);
}

void v2_piece_verifier::verify_finished_piece(const v2_hashed_piece& finished_piece) {
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
        verify_piece_layers_and_root(*hasher, finished_piece.file_index);
    }
}

template<typename HasherType>
void v2_piece_verifier::verify_piece_layers_and_root(HasherType& hasher, std::size_t file_index) {
    file_storage& storage = storage_;
    std::size_t piece_size = storage.piece_size();

    auto& tree = merkle_trees_[file_index];

    // complete the merkle tree
    tree.update(hasher);

    // the depth in the tree of hashes covering blocks of size `piece_size`
    auto layer_offset = detail::log2_floor(piece_size) - detail::log2_floor(v2_block_size);

    // verify the root hash
    file_entry& entry = storage.at(file_index);
    std::size_t entry_index = file_offsets_.at(file_index);

    bool whole_file_complete = entry.pieces_root() == tree.root();
    bool is_single_piece_file = entry.piece_layer().size() == 0;


    if (whole_file_complete) {
        if (is_single_piece_file) {
            Expects(entry_index < piece_map_.size());
            piece_map_[entry_index] = 1;
        }
        else {
            std::fill_n(std::next(piece_map_.begin(), entry_index), entry.piece_layer().size(), 1);
        }
    }
    else {
        if (is_single_piece_file) {
            Expects(entry_index < piece_map_.size());
            piece_map_[entry_index] = 0;
        }
        else {

            // leaf nodes necessary to balance the tree are not included in the piece layers
            std::size_t layer_depth = tree.tree_height() - layer_offset;
            auto layer_view = tree.get_layer(layer_depth);
            auto layer_data_nodes_size = (entry.file_size() + piece_size - 1 ) / piece_size;
            layer_view = layer_view.subspan(0, layer_data_nodes_size);
            auto reference_layer_view = entry.piece_layer();

            std::size_t max_offset = std::min(layer_view.size(), reference_layer_view.size());

            for (std::size_t i = 0; i < max_offset; ++i, ++entry_index) {
                piece_map_[entry_index] = (layer_view[i] == reference_layer_view[i]);
            }
        }
    }
}

}
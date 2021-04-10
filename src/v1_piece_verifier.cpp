#include "dottorrent/v1_piece_verifier.hpp"

namespace dottorrent {

v1_piece_verifier::v1_piece_verifier(
        dottorrent::file_storage& storage, std::size_t capacity, std::size_t max_concurrency)
        : storage_(storage)
        , processor_(capacity)
        , piece_map_(storage.piece_count(), false)
{
    processor_.set_work_function([this](const v1_hashed_piece& p) { this->verify_finished_piece(p); });
    processor_.set_max_concurrency(max_concurrency);
}

void v1_piece_verifier::start() {
    processor_.start();
}

void v1_piece_verifier::request_cancellation() {
    processor_.request_cancellation();
}

void v1_piece_verifier::request_stop() {
    processor_.request_stop();
}

void v1_piece_verifier::wait() {
    processor_.wait();
}

std::shared_ptr<hashed_piece_processor::v1_piece_queue_type> v1_piece_verifier::get_v1_queue() {
    return processor_.get_queue();
}

std::shared_ptr<hashed_piece_processor::v2_piece_queue_type> v1_piece_verifier::get_v2_queue() {
    throw std::logic_error("not implemented");
}

const std::vector<std::uint8_t>& v1_piece_verifier::result() const noexcept {
    return piece_map_;
}

double v1_piece_verifier::percentage(std::size_t file_index) const noexcept {
    file_storage& storage = storage_;
    auto [first, last] = storage.get_pieces_offsets(file_index);
    auto number_of_pieces = last-first;

    if (number_of_pieces == 0) [[unlikely]]
        return 100;

    auto n_complete_pieces = std::count(std::next(piece_map_.begin(), first),
            std::next(piece_map_.begin(), last), 1);
    return double(n_complete_pieces) / double(number_of_pieces) * 100;
}

void v1_piece_verifier::verify_finished_piece(const v1_hashed_piece& piece) {
    file_storage& storage = storage_;
    const auto& real_hash = storage.get_piece_hash(piece.index);
    piece_map_[piece.index] = (real_hash == piece.hash);
}

}
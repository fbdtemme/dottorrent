#include "dottorrent/v1_piece_writer.hpp"

namespace dottorrent {

v1_piece_writer::v1_piece_writer(file_storage& storage, std::size_t capacity, std::size_t max_concurrency)
        : storage_(storage)
        , processor_(capacity)
{
    processor_.set_max_concurrency(max_concurrency);
    processor_.set_work_function([this](const v1_hashed_piece& p) { this->set_finished_piece(p); });
}

void v1_piece_writer::start() {
    processor_.start();
}

void v1_piece_writer::request_cancellation() {
    processor_.request_cancellation();
}

void v1_piece_writer::request_stop() {
    processor_.request_stop();
}

void v1_piece_writer::wait() {
    processor_.wait();
}

std::shared_ptr<v1_piece_writer::v1_piece_queue_type> v1_piece_writer::get_v1_queue() {
    return processor_.get_queue();
}

std::shared_ptr<v1_piece_writer::v2_piece_queue_type> v1_piece_writer::get_v2_queue() {
    throw std::logic_error("not implemented");
}

void v1_piece_writer::set_finished_piece(const v1_hashed_piece& finished_piece) {
    file_storage& storage = storage_.get();
    storage.set_piece_hash(finished_piece.index, finished_piece.hash);
}
}
#pragma once

#include <optional>

#include "concurrent_queue.hpp"
#include "file_storage.hpp"
#include "hash.hpp"
#include "concurrent_queue_processor.hpp"
#include "hashed_piece_processor.hpp"

namespace dottorrent {

class v1_piece_writer : public hashed_piece_processor
{
public:
    v1_piece_writer(file_storage& storage, std::size_t capacity, std::size_t max_concurrency = 1);

    void start() override;

    void request_cancellation() override;

    void request_stop() override;

    void wait() override;

    std::shared_ptr<v1_piece_queue_type> get_v1_queue() override;

    std::shared_ptr<v2_piece_queue_type> get_v2_queue() override;

private:
    void set_finished_piece(const v1_hashed_piece& finished_piece);

    std::reference_wrapper<file_storage> storage_;
    concurrent_queue_processor<v1_hashed_piece> processor_;
};

}
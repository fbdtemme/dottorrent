#pragma once

#include <optional>

#include "concurrent_queue.hpp"
#include "file_storage.hpp"
#include "hash.hpp"
#include "concurrent_queue_processor.hpp"
#include "hashed_piece.hpp"
#include "hashed_piece_processor.hpp"
#include "hashed_piece_verifier.hpp"

namespace dottorrent {

class v1_piece_verifier : public hashed_piece_verifier
{
public:
    v1_piece_verifier(file_storage& storage, std::size_t capacity, std::size_t max_concurrency = 1);

    void start() override;

    void request_cancellation() override;

    void request_stop() override;

    void wait() override;

    bool running() const noexcept override;

    bool started() const noexcept override;

    bool cancelled() const noexcept override;

    bool done() const noexcept override;

    std::shared_ptr<v1_piece_queue_type> get_v1_queue() override;

    std::shared_ptr<v2_piece_queue_type> get_v2_queue() override;

    const std::vector<std::uint8_t>& result() const noexcept override;

    double percentage(std::size_t file_index) const noexcept override;

private:
    void verify_finished_piece(const v1_hashed_piece& piece);

    std::reference_wrapper<file_storage> storage_;
    concurrent_queue_processor<v1_hashed_piece> processor_;
    std::vector<std::uint8_t> piece_map_;

};

}
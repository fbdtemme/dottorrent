#pragma once

#include <chrono>
#include "dottorrent/chunk_processor.hpp"

namespace dottorrent {


class chunk_processor_base : public chunk_processor
{
    using work_queue_type = typename chunk_processor::queue_type;
    using v1_hashed_piece_queue = typename chunk_processor::v1_hashed_piece_queue;
    using v2_hashed_piece_queue = typename chunk_processor::v2_hashed_piece_queue;

public:
    explicit chunk_processor_base(file_storage& storage,
            std::vector<hash_function> hf,
            std::size_t capacity,
            std::size_t thread_count = 1);

    chunk_processor_base(const chunk_processor_base& other) = delete;
    chunk_processor_base(chunk_processor_base&& other) = delete;

    chunk_processor_base& operator=(const chunk_processor_base& other) = delete;
    chunk_processor_base& operator=(chunk_processor_base&& other) = delete;

    /// Start the worker threads
    void start() override;

    /// Signal the workers to shut down after finishing all pending work.
    /// No items can be added to the work queue after this call.
    void request_stop() override;

    /// Signal the workers to shut down and discard pending work.
    void request_cancellation() override;

    /// Block until all workers finish their execution.
    /// Note that workers will only finish after a call to stop or cancel.
    void wait() override;

    bool running() const noexcept override;

    /// Return true if the thread has been started.
    auto started() const noexcept -> bool;

    auto cancelled() const noexcept -> bool;

    /// Check if the hasher has completed all work or is cancelled.
    auto done() const noexcept -> bool;

    std::shared_ptr<work_queue_type> get_queue() override;

    std::shared_ptr<const work_queue_type> get_queue() const override;

    void register_v1_hashed_piece_queue(const std::shared_ptr<v1_hashed_piece_queue>& queue) override;

    void register_v2_hashed_piece_queue(const std::shared_ptr<v2_hashed_piece_queue>& queue) override;

    /// Number of total bytes hashes.
    auto bytes_hashed() const noexcept -> std::size_t;

    /// Number of bytes processed.
    auto bytes_done() const noexcept -> std::size_t;

    ~chunk_processor_base() override;

protected:
    virtual void run(std::stop_token stop_token, int thread_idx) = 0;

    std::reference_wrapper<file_storage> storage_;
    std::vector<std::jthread> threads_;
    std::shared_ptr<work_queue_type> queue_;
    std::shared_ptr<v1_hashed_piece_queue> v1_hashed_piece_queue_;
    std::shared_ptr<v2_hashed_piece_queue> v2_hashed_piece_queue_;

    std::vector<hash_function> hash_functions_;

    std::atomic<bool> started_ = false;
    std::atomic<bool> cancelled_ = false;
    std::atomic<bool> stopped_ = false;
    std::vector<std::atomic<bool>> done_;

    // The amount of bytes that were actually hashed.
    std::atomic<std::size_t> bytes_hashed_ = 0;
    // How many pieces were processed (some pieces can be processed without hashing).
    std::atomic<std::size_t> bytes_done_ = 0;
};

} // namespace dottorrent
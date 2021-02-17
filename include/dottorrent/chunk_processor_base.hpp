#pragma once

#include <chrono>
#include "dottorrent/chunk_processor.hpp"

namespace dottorrent {

class chunk_processor_base : public chunk_processor
{
public:
    explicit chunk_processor_base(file_storage& storage,
            std::vector<hash_function> hf,
            std::size_t capacity,
            std::size_t thread_count = 1)
        : storage_(storage)
        , threads_(thread_count)
        , queue_(std::make_shared<queue_type>(capacity))
        , hash_functions_(std::move(hf))
        , done_(thread_count)
    { }

    chunk_processor_base(const chunk_processor_base& other) = delete;
    chunk_processor_base(chunk_processor_base&& other) = delete;

    chunk_processor_base& operator=(const chunk_processor_base& other) = delete;
    chunk_processor_base& operator=(chunk_processor_base&& other) = delete;

    /// Start the worker threads
    void start() override
    {
        Ensures(!started());
        Ensures(!cancelled());

        for (std::size_t i = 0; i < threads_.size(); ++i) {
            threads_[i] = std::jthread(&chunk_processor_base::run, this, i);
            done_[i] = false;
        }
        started_.store(true, std::memory_order_release);
    }

    /// Signal the workers to shut down after finishing all pending work.
    /// No items can be added to the work queue after this call.
    void request_stop() override
    {
        rng::for_each(threads_, [](std::jthread& t) { t.request_stop(); });
    }

    /// Signal the workers to shut down and discard pending work.
    void request_cancellation() override
    {
        cancelled_.store(true, std::memory_order_relaxed);
        request_stop();
    }

    /// Block until all workers finish their execution.
    /// Note that workers will only finish after a call to stop or cancel.
    void wait() override
    {
        if (!started()) return;

        for (auto i = 0; i < threads_.size(); ++i) {
            queue_->push({
                    std::numeric_limits<std::uint32_t>::max(),
                    std::numeric_limits<std::uint32_t>::max(),
                    nullptr
            });
        }

        while (!done()) {
            queue_->push({
                    std::numeric_limits<std::uint32_t>::max(),
                    std::numeric_limits<std::uint32_t>::max(),
                    nullptr
            });
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // should all be finished by now and ready to join
        for (auto& t : threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    bool running() const noexcept override
    {
        return started_.load(std::memory_order_relaxed) &&
                !cancelled_.load(std::memory_order_relaxed) &&
                !stopped_.load(std::memory_order_relaxed);
    }

    /// Return true if the thread has been started.
    auto started() const noexcept -> bool
    { return started_.load(std::memory_order_relaxed); }

    auto cancelled() const noexcept -> bool
    { return cancelled_.load(std::memory_order_relaxed); }

    /// Check if the hasher has completed all work or is cancelled.
    auto done() const noexcept -> bool
    {
        bool all_done = std::all_of(done_.begin(), done_.end(),
                [](auto& b) { return b.load(std::memory_order_relaxed); });
        return cancelled() || (started() && all_done);
    }

    std::shared_ptr<queue_type> get_queue() override
    { return queue_; }

    std::shared_ptr<const queue_type> get_queue() const override
    { return queue_; }

    /// Number of total bytes hashes.
    auto bytes_hashed() const noexcept -> std::size_t
    { return bytes_hashed_.load(std::memory_order_relaxed); }

    /// Number of bytes processed.
    auto bytes_done() const noexcept -> std::size_t
    { return bytes_done_.load(std::memory_order_relaxed); }

    ~chunk_processor_base() override = default;

protected:
    virtual void run(int thread_idx) = 0;

    std::reference_wrapper<file_storage> storage_;
    std::vector<std::jthread> threads_;
    std::shared_ptr<queue_type> queue_;
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
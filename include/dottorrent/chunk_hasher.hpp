#pragma once
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <ranges>

#include <gsl/gsl_assert>
#include <tbb/concurrent_queue.h>

#include "dottorrent/hash.hpp"
#include "dottorrent/file_storage.hpp"
#include "dottorrent/data_chunk.hpp"

namespace dottorrent {

namespace rng = std::ranges;

template <typename H>
    requires std::is_base_of_v<hasher, H>
class chunk_hasher
{
public:
    using chunk_type = data_chunk;
    using hasher_type = H;
    using hash_type = typename hasher_type::hash_type;
    using queue_type = tbb::concurrent_bounded_queue<chunk_type>;

    explicit chunk_hasher(file_storage& storage, std::size_t thread_count)
            : storage_(storage)
            , threads_(thread_count)
            , queue_(std::make_shared<queue_type>())
    { }

    chunk_hasher(const chunk_hasher& other) = delete;
    chunk_hasher(chunk_hasher&& other) = delete;
    chunk_hasher& operator=(const chunk_hasher& other) = delete;
    chunk_hasher& operator=(chunk_hasher&& other) = delete;

    /// Start the worker threads
    void start()
    {
        Ensures(!started());
        Ensures(!cancelled());

        for (std::size_t i = 0; i < threads_.size(); ++i) {
            threads_[i] = std::jthread(&chunk_hasher::run, this, i);
        }
        started_.store(true, std::memory_order_release);
    }

    /// Signal the workers to shut down after finishing all pending work.
    /// No items can be added to the work queue after this call.
    void request_stop()
    {
        rng::for_each(threads_, [](std::jthread& t) { t.request_stop(); });
    }

    /// Signal the workers to shut down and discard pending work.
    void request_cancellation()
    {
        cancelled_.store(true, std::memory_order_relaxed);
        request_stop();
    }

    /// Block until all workers finish their execution.
    /// Note that workers will only finish after a call to stop or cancel.
    void wait()
    {
        if (!started()) return;

        // Wake up all threads with a poison pill if they are blocked on pop()
        // tbb::concurrent_queue has negative size if pop() 's are pending.
        for (auto i = 0; i < 2 * threads_.size()+1; ++i) {
            queue_->push({
                std::numeric_limits<std::uint32_t>::max(),
                std::numeric_limits<std::uint32_t>::max(),
                nullptr
            });
        }

        for (auto& t : threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    auto running() -> bool
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

    /// Check if the hasher is completed or cancelled.
    auto done() const noexcept -> bool
    {
        return cancelled() || (started() && std::none_of(
                threads_.begin(), threads_.end(),
                [](const auto& t) { return t.joinable(); }));
    }

    auto get_queue() -> const std::shared_ptr<queue_type>&
    { return queue_; }

    /// Number of total bytes processed.
    auto bytes_hashed() const noexcept -> std::size_t
    { return bytes_hashed_.load(std::memory_order_relaxed); }

    /// Number of pieces processed.
    auto pieces_done() const noexcept -> std::size_t
    { return pieces_done_.load(std::memory_order_relaxed); }

protected:
    virtual void run(int thread_idx)
    {
        hasher_type hasher {};
        data_chunk item {};
        auto stop_token = threads_[thread_idx].get_stop_token();

        // Process tasks until stopped is set
        while (!stop_token.stop_requested() && stop_token.stop_possible()) {
            queue_->pop(item);
            // check if the data_chunk is valid or a stop wake-up signal
            if (item.data == nullptr && item.piece_index == -1 && item.file_index == -1) {
                continue;
            }

            hash_chunk(hasher, item);
            item.data.reset();
        }

        // finish pending tasks if the hasher is not cancelled,
        // otherwise discard all remaining work
        if (!cancelled_.load(std::memory_order_relaxed)) {
            while (queue_->try_pop(item)) {
                if (item.data == nullptr && item.piece_index == -1 && item.file_index == -1) {
                    break;
                }
                hash_chunk(hasher, item);
            }
        }
    }

    virtual void hash_chunk(hasher_type& hasher, const data_chunk& chunk) = 0;

protected:
    std::reference_wrapper<file_storage> storage_;
    std::vector<std::jthread> threads_;
    std::shared_ptr<queue_type> queue_;

    std::atomic<bool> started_ = false;
    std::atomic<bool> cancelled_ = false;
    std::atomic<bool> stopped_ = false;

    // The amount of bytes that were hashed.
    std::atomic<std::size_t> bytes_hashed_ = 0;
    // How many pieces were processed (some pieces can be processed without hashing).
    std::atomic<std::size_t> pieces_done_ = 0;
};


} // namespace dottorrent
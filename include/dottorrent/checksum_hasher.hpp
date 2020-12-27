#pragma once
#include <vector>
#include <memory>
#include <atomic>
#include <thread>

#include <gsl-lite/gsl-lite.hpp>
#include <tbb/concurrent_queue.h>

#include "dottorrent/hash_function.hpp"
#include "dottorrent/hash.hpp"
#include "dottorrent/checksum.hpp"

#include "dottorrent/file_storage.hpp"
#include "dottorrent/data_chunk.hpp"
#include "dottorrent/hash_function_traits.hpp"
#include "dottorrent/chunk_hasher.hpp"
#include "dottorrent/hasher/factory.hpp"

namespace dottorrent {

/// Only useful for v1 torrents.
/// Per file merkle root checksums are in the base v2 spec.


class checksum_hasher
{
public:
    using chunk_type = data_chunk;
    using queue_type = tbb::concurrent_bounded_queue<chunk_type>;

    explicit checksum_hasher(hash_function f, file_storage& storage)
            : storage_(storage)
            , queue_(std::make_shared<queue_type>())
            , hash_function_(f)
            , hasher_(make_hasher(f))
    {};

    /// Start the worker threads
    void start()
    {
        Ensures(!started());
        Ensures(!cancelled());

        thread_ = std::jthread(&checksum_hasher::run, this);
        started_.store(true, std::memory_order_release);
    }

    /// Signal the workers to shut down after finishing all pending work.
    /// No items can be added to the work queue after this call.
    void request_stop()
    {
        thread_.request_stop();
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

        // Wake up threads with a poison pill if they are blocked on pop()
        // tbb::concurrent_queue has negative size if pop() 's are pending.
        while (queue_->size() < 0) {
            queue_->push({0, 0, nullptr});
        }

        if (thread_.joinable()) {
            thread_.join();
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
    { return cancelled() || (started() && !thread_.joinable()); }

    auto get_queue() -> const std::shared_ptr<queue_type>&
    { return queue_; }

    /// Number of total bytes processed.
    auto bytes_hashed() const noexcept -> std::size_t
    { return bytes_hashed_.load(std::memory_order_relaxed); }

    auto progress() -> double
    {
        return this->bytes_hashed() / this->storage_.get().total_file_size();
    }

private:
    virtual void run()
    {
        auto hasher = make_hasher(hash_function_);

        data_chunk item {};
        auto stop_token = thread_.get_stop_token();

        // Process tasks until stopped is set
        while (!stop_token.stop_requested() && stop_token.stop_possible()) {
            queue_->pop(item);
            // check if the data_chunk is valid or a stop wake up signal chunk
            // check if the data_chunk is valid or a stop wake-up signal
            if (item.data == nullptr && item.piece_index == -1 && item.file_index == -1) {
                continue;
            }

            hash_chunk(*hasher, item);
            item.data.reset();
        }

        // finish pending tasks if the hasher is not cancelled,
        // otherwise discard all remaining work
        if (!cancelled_.load(std::memory_order_relaxed)) {
            while (queue_->try_pop(item)) {
                if (item.data == nullptr && item.piece_index == -1 && item.file_index == -1) {
                    break;
                }
                hash_chunk(*hasher_, item);
            }
        }
    }

    void hash_chunk(hasher& hasher, const data_chunk& item)
    {
        // TODO: test
        file_storage& storage = this->storage_;
        auto data = std::span(*item.data);
        std::size_t chunk_size = data.size();
        std::size_t chunk_bytes_processed = 0;
        current_file_size_ = storage[current_file_idx_].file_size();

        // Verify that we are receive chunks sequentially
        Ensures(current_file_idx_ == item.file_index);

        while (chunk_bytes_processed < chunk_size) {
            auto curr_file_remaining = current_file_size_ - current_file_data_hashed_;
            auto curr_file_subchunk_size = std::min(curr_file_remaining, chunk_size);
            hasher.update(data.subspan(chunk_bytes_processed, curr_file_subchunk_size));
            chunk_bytes_processed += curr_file_subchunk_size;
            current_file_data_hashed_ += curr_file_subchunk_size;
            bytes_hashed_.fetch_add(curr_file_subchunk_size, std::memory_order_relaxed);

            // check if we completed a file checksum
            if (current_file_data_hashed_ == current_file_size_) {
                auto checksum = make_checksum(hash_function_);
                hasher.finalize_to(checksum->value());
                storage[current_file_idx_].add_checksum(std::move(checksum));

                ++current_file_idx_;

                // reset file counters
                if (current_file_size_ < storage.file_count()) {
                    current_file_size_ = storage[current_file_idx_].file_size();
                } else {
                    current_file_size_ = 0;
                }
                current_file_data_hashed_ = 0;
            }
        }
    }

private:
    std::reference_wrapper<file_storage> storage_;
    std::shared_ptr<queue_type> queue_;
    std::jthread thread_;
    hash_function hash_function_;
    std::unique_ptr<hasher> hasher_;

    std::atomic<std::size_t> bytes_hashed_;
    std::atomic<bool> started_ = false;
    std::atomic<bool> cancelled_ = false;
    std::atomic<bool> stopped_ = false;

    std::size_t current_file_idx_ = 0;
    std::size_t current_file_size_ = 0;
    std::size_t current_file_data_hashed_ = 0;
};

//
//using md5_checksum_hasher = basic_checksum_hasher<hash_function::md5>;
//using sha1_checksum_hasher = basic_checksum_hasher<hash_function::sha1>;
//using sha256_checksum_hasher = basic_checksum_hasher<hash_function::sha256>;

//
//
//class checksum_hasher : chunk_hasher
//{
//    using chunk_type = data_chunk;
//    using queue_type = tbb::concurrent_bounded_queue<chunk_type>;
//
//    checksum_hasher(file_storage& storage, hash_function f)
//            : storage_(storage)
//    {
//        hash_function
//    }
//
//
//    hasher_type
//    std::reference_wrapper<file_storage> storage_;
//    std::jthread thread_;
//};

}
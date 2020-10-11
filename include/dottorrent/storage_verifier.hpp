#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <mutex>
#include <numeric>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include <bit>

#include <gsl/gsl_assert>
#include <tbb/concurrent_queue.h>

#include <dottorrent/object_pool.hpp>

#include "dottorrent/literals.hpp"
#include "dottorrent/file_entry.hpp"
#include "dottorrent/file_storage.hpp"

#include "dottorrent/data_chunk.hpp"
#include "dottorrent/v1_chunk_reader.hpp"
#include "dottorrent/v1_chunk_verifier.hpp"
#include "dottorrent/v2_chunk_reader.hpp"
#include "dottorrent/v2_chunk_verifier.hpp"

#include "dottorrent/hash.hpp"
#include "dottorrent/checksum.hpp"
#include "dottorrent/checksum_hasher.hpp"



namespace dottorrent {

namespace fs = std::filesystem;
using namespace dottorrent::literals;
using namespace std::chrono_literals;


/// verify pieces.
/// First compare file sizes. Only check data for files with exact file size.
/// When piece contains data from two files of which one is missing this piece will be incomplete.
// class verify_v1 : public hasher_base<sha1_hasher> {
//
//}


/// Options to control the memory usage of a storage_hasher.
struct storage_verifier_options
{
    /// The bittorrent procolol version to verify chunks for.
    /// Hybrid will verify both v1 and v2 hashes.
    protocol protocol_version = protocol::v1;
    /// The minimum size of a block to read from disk.
    /// For piece sizes smaller than the min_chunk_size multiple pieces
    /// will be read in a single block for faster disk I/O.
    std::size_t min_chunk_size = 1_MiB;
    /// Max size of all file chunks in memory
    std::size_t max_memory = 128_MiB;

    /// Number of threads to hash pieces.
    /// Total number of threads will be equal to:
    /// 1 main thread + 1 reader + <#threads> piece hashers + <#checksums types> checsum hashers
    std::size_t threads = 2;
};



class storage_verifier
{
public:
    /// Options to control the memory usage of a storage_hasher.
    struct memory_options
    {
        std::size_t min_chunk_size;
        std::size_t max_memory;
    };

    explicit storage_verifier(
            file_storage& storage,
            const storage_verifier_options& options = {})
            : storage_(storage)
            , protocol_(options.protocol_version)
            , memory_({options.min_chunk_size, options.max_memory})
            , threads_(options.threads)
            , reader_()
    {
        Expects(std::has_single_bit(storage.piece_size()));
        cumulative_file_size_ = exclusive_file_size_scan(storage);
    }

    /// Return a reference to the file_storage object.
    file_storage& storage()
    { return storage_.get(); }

    const file_storage& storage() const
    { return storage_; }

    void start()
    {
        if (done())
            throw std::runtime_error(
                    "cannot start finished or cancelled hasher");

        auto& storage = storage_.get();
        auto chunk_size = std::max(memory_.min_chunk_size, storage.piece_size());

        if (protocol_ == protocol::v1) {
            reader_ = std::make_unique<v1_chunk_reader>(storage_, chunk_size, memory_.max_memory);
        }
        else {
            reader_ = std::make_unique<v2_chunk_reader>(storage_, chunk_size, memory_.max_memory);
        }
        if (protocol_ == protocol::v1) {
            v1_verifier_ = std::make_unique<v1_chunk_verifier>(storage_, threads_);
            reader_->register_hash_queue(v1_verifier_->get_queue());
        }

        if (protocol_ == protocol::v2 || protocol_ == protocol::hybrid) {
            v2_verifier_ = std::make_unique<v2_chunk_verifier>(storage_, threads_);
            reader_->register_hash_queue(v2_verifier_->get_queue());
        }


        if (v1_verifier_) v2_verifier_->start();
        if (v2_verifier_) v2_verifier_->start();

        reader_->start();
        started_ = true;

        if (v1_verifier_) v1_verifier_->start();
        if (v2_verifier_) v2_verifier_->start();

        reader_->start();
        started_ = true;
    }

    void cancel()
    {
        if (done()) {
            throw std::runtime_error("cannot cancel finished or cancelled hasher");
        }
        if (!started_) {
            cancelled_ = true;
            return;
        }

        // cancel all tasks
        reader_->request_cancellation();
        if (v1_verifier_)
            v1_verifier_->request_cancellation();
        if (v2_verifier_)
            v2_verifier_->request_cancellation();

        // wait for all tasks to complete
        if (v1_verifier_)
            v1_verifier_->wait();
        if (v2_verifier_)
            v2_verifier_->wait();

        reader_->wait();
        cancelled_ = true;
        stopped_ = true;
    }

    /// Wait for all threads to complete
    void wait()
    {
        if (!started())
            throw std::runtime_error("hasher not running");
        if (done())
            throw std::runtime_error("hasher already done");

        reader_->wait();
        // no pieces will be added after the reader finishes.
        // So we can signal hashers that they can shutdown after
        // finishing all remaining work
        if (v1_verifier_)
            v1_verifier_->request_stop();
        if (v2_verifier_)
            v2_verifier_->request_stop();

        if (v1_verifier_)
            v1_verifier_->wait();
        if (v2_verifier_)
            v2_verifier_->wait();

        stopped_ = true;
    }

    auto running() const -> bool
    { return started_ && !cancelled_ && !stopped_; }

    auto started() const -> bool
    { return started_; }

    auto cancelled() const -> bool
    { return cancelled_; }

    /// Check if the hasher is still processing tasks or is just waiting.
    auto done() const -> bool
    { return cancelled_ || (started_ && stopped_); }

    auto bytes_read() const -> std::size_t
    { return reader_->bytes_read(); }

    /// Return the number of bytes processed by the v1 or v2 hasher,
    /// or the average between the two for hybrid torrents.
    auto bytes_verified() const noexcept -> std::size_t
    {
        const auto& storage = storage_.get();
        switch (protocol_) {
            case protocol::v1: {
                if (v1_verifier_ == nullptr) { return 0; }
                return v1_verifier_->pieces_done() * storage.piece_size();
            }
            case protocol::v2: {
                if (v2_verifier_ == nullptr) { return 0; }
                return v2_verifier_->pieces_done() * storage.piece_size();
            }
            case protocol::hybrid: {
                return (v2_verifier_->pieces_done()+v2_verifier_->pieces_done())
                        /2*storage.piece_size();
            }
            default:
                Ensures(false);
        }
    }

    /// Get information about the file currently being hashed.
    /// @returns A pair with a pointer to the file_entry and
    ///     the number of bytes hashed for this file
    struct file_progress_data
    {
        std::size_t file_index;
        std::size_t bytes_hashed;
    };

    auto current_file_progress() const -> file_progress_data
    {
        const auto& storage = storage_.get();
        const auto bytes = bytes_verified();

        auto it = std::lower_bound(
                std::next(cumulative_file_size_.begin(), current_file_index_),
                cumulative_file_size_.end(),
                bytes);
        auto index = std::distance(cumulative_file_size_.begin(), it);
        current_file_index_ = static_cast<std::size_t>(index);

        if (index == 0) {
            return {current_file_index_, bytes};
        }
        else {
            return {current_file_index_, bytes-*std::next(it, -1)};
        }
    }

    auto result()
    {
        return v1_verifier_->result();
    }

private:
    std::reference_wrapper<file_storage> storage_;
    protocol protocol_;
    memory_options memory_;
    std::size_t threads_;

    std::unique_ptr<chunk_reader> reader_;
    std::unique_ptr<v1_chunk_verifier> v1_verifier_;
    std::unique_ptr<v2_chunk_verifier> v2_verifier_;

    bool started_ = false;
    bool stopped_ = false;
    bool cancelled_ = false;

    // progress info
    mutable std::size_t current_file_index_ = 0;
    std::vector<std::size_t> cumulative_file_size_;
};

} // namespace dottorrent
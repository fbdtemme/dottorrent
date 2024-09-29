//
// Created by fbdtemme on 9/11/20.
//
#include <bit>

#include "dottorrent/storage_hasher.hpp"

#include "dottorrent/v1_chunk_reader.hpp"
#include "dottorrent/v2_chunk_reader.hpp"

#include "dottorrent/v1_chunk_hasher_sb.hpp"
#include "dottorrent/v1_chunk_hasher_mb.hpp"
#include "dottorrent/v2_chunk_hasher_sb.hpp"
#include "dottorrent/v2_chunk_hasher_mb.hpp"

#include "dottorrent/v1_piece_writer.hpp"
#include "dottorrent/v2_piece_writer.hpp"

#include <dottorrent/v1_checksum_hasher.hpp>
#include <dottorrent/v2_checksum_hasher.hpp>


namespace dottorrent {

storage_hasher::storage_hasher(file_storage& storage, const storage_hasher_options& options)
        : storage_(storage)
        , protocol_(options.protocol_version)
        , checksums_(options.checksums)
        , threads_(options.threads)
        , io_block_size_()
        , queue_capacity_()
        , enable_multi_buffer_hashing_(options.enable_multi_buffer_hashing)
{
    if (storage.piece_size() == 0)
        storage.set_piece_size(choose_piece_size(storage));

    auto piece_size = storage.piece_size();

#ifdef DOTTORRENT_USE_ISAL_CRYPTO
    if (options.min_io_block_size) {
        io_block_size_ = std::max(piece_size, *options.min_io_block_size);
    } else {
        io_block_size_ = 16 * piece_size;
    }
#else
    io_block_size_ = std::max(piece_size, options.min_io_block_size ? *options.min_io_block_size : 1_MiB);
 #endif

    if (options.max_memory) {
        queue_capacity_ = std::max(std::size_t(4), *options.max_memory / io_block_size_);
    }
    else {
        queue_capacity_ = std::max(std::size_t(4), 4 * threads_);
    }

    Expects(protocol_ != dottorrent::protocol::none);
    Expects(std::has_single_bit(storage.piece_size()));    // is a power of 2

    if (protocol_ == protocol::hybrid) {
        // add v1 padding files
        optimize_alignment(storage_);
    }

    // allocate the required pieces in storage objects
    if (protocol_ == protocol::v1 || protocol_ == protocol::hybrid) {
        storage.allocate_pieces();
    }

    if (protocol_ == protocol::v1) {
        cumulative_file_size_ = inclusive_file_size_scan_v1(storage);
    }
    else {
        cumulative_file_size_ = inclusive_file_size_scan_v2(storage);
    }
}

file_storage& storage_hasher::storage()
{
    return storage_.get();
}

const file_storage& storage_hasher::storage() const
{
    return storage_;
}

enum protocol storage_hasher::protocol() const noexcept
{
    return protocol_;
}


void storage_hasher::start() {
    if (done())
        throw std::runtime_error("cannot start finished or cancelled hasher");

    auto& storage = storage_.get();

    if (protocol_ == protocol::v1) {
        reader_ = std::make_unique<v1_chunk_reader>(storage_, io_block_size_, queue_capacity_);
    }
    else {
        reader_ = std::make_unique<v2_chunk_reader>(storage_, io_block_size_, queue_capacity_);
    }

    if (protocol_ == protocol::v1) {
#ifdef DOTTORRENT_USE_ISAL_CRYPTO
        if (enable_multi_buffer_hashing_)
            hasher_ = std::make_unique<v1_chunk_hasher_mb>(storage_, queue_capacity_, threads_);
        else
            hasher_ = std::make_unique<v1_chunk_hasher_sb>(storage_, queue_capacity_, threads_);
#else
        hasher_ = std::make_unique<v1_chunk_hasher_sb>(storage_, queue_capacity_, threads_);
#endif
        reader_->register_hash_queue(hasher_->get_queue());

        for (auto algo : checksums_) {
            auto& h = checksum_hashers_.emplace_back(
                    std::make_unique<v1_checksum_hasher>(storage_, algo, queue_capacity_));
            reader_->register_checksum_queue(h->get_queue());
        }

        verifier_ = std::make_unique<v1_piece_writer>(storage_, -1, 1);
        hasher_->register_v1_hashed_piece_queue(verifier_->get_v1_queue());
    }
    else {

#ifdef DOTTORRENT_USE_ISAL_CRYPTO
        if (enable_multi_buffer_hashing_)
            hasher_ = std::make_unique<v2_chunk_hasher_mb>(
                    storage_, queue_capacity_, protocol_ == protocol::hybrid, threads_);
        else
            hasher_ = std::make_unique<v2_chunk_hasher_sb>(
                    storage_, queue_capacity_, protocol_ == protocol::hybrid, threads_);
#else
        hasher_ = std::make_unique<v2_chunk_hasher_sb>(
                storage_, queue_capacity_, protocol_ == protocol::hybrid, threads_);
#endif
        reader_->register_hash_queue(hasher_->get_queue());

        for (auto algo : checksums_) {
            auto& h = checksum_hashers_.emplace_back(
                    std::make_unique<v2_checksum_hasher>(storage_, algo, queue_capacity_));
            reader_->register_checksum_queue(h->get_queue());
        }

        verifier_ = std::make_unique<v2_piece_writer>(storage_, -1, protocol_ == protocol::hybrid, 1);
        hasher_->register_v1_hashed_piece_queue(verifier_->get_v1_queue());
        hasher_->register_v2_hashed_piece_queue(verifier_->get_v2_queue());
    }

    // start all parts
    verifier_->start();
    hasher_->start();
    for (auto& ch : checksum_hashers_) { ch->start(); }
    reader_->start();
    started_ = true;
}

void storage_hasher::cancel() {
    if (done()) {
        throw std::runtime_error(
                "cannot cancel finished or cancelled hasher");
    }
    if (!started_) {
        cancelled_ = true;
        return;
    }

    // cancel all tasks
    reader_->request_cancellation();
    hasher_->request_cancellation();
    for (auto& ch : checksum_hashers_) { ch->request_cancellation(); }
    verifier_->request_cancellation();

    // wait for all tasks to complete
    reader_->wait();
    hasher_->wait();
    for (auto& ch : checksum_hashers_) { ch->wait(); }
    verifier_->wait();

    cancelled_ = true;
    stopped_ = true;
}

void storage_hasher::wait()
{
    if (!started())
        throw std::runtime_error("hasher not running");
    if (done())
        throw std::runtime_error("hasher already done");

    Expects(hasher_->running());
    reader_->wait();
    Expects(hasher_->running());

    // no pieces will be added after the reader finishes.
    // So we can signal hashers that they can shutdown after
    // finishing all remaining work
    hasher_->request_stop();
    for (auto& ch : checksum_hashers_) { ch->request_stop(); }
    hasher_->wait();
    for (auto& ch : checksum_hashers_) { ch->wait(); }

    verifier_->request_stop();
    verifier_->wait();

    stopped_ = true;
}

bool storage_hasher::running() const noexcept
{
    return started_ && !cancelled_ && !stopped_;
}

bool storage_hasher::started() const noexcept
{
    return started_;
}

bool storage_hasher::cancelled() const noexcept
{
    return cancelled_;
}

bool storage_hasher::done() const noexcept
{
    return cancelled_ || (started_ && stopped_);
}

std::size_t storage_hasher::bytes_read() const noexcept
{
    return reader_->bytes_read();
}

std::size_t storage_hasher::bytes_hashed() const noexcept
{
    return hasher_->bytes_hashed();
}

std::size_t storage_hasher::bytes_done() const noexcept
{
    return hasher_->bytes_done();
}


file_progress_data storage_hasher::current_file_progress() const noexcept
{
    const auto& storage = storage_.get();
    const auto bytes = bytes_done();

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
        return {current_file_index_, bytes-*std::next(it, - 1)};
    }
}

} //namespace dottorrent
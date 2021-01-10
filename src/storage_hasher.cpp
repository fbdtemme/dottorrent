//
// Created by fbdtemme on 9/11/20.
//
#include <bit>
#include "dottorrent/storage_hasher.hpp"


namespace dottorrent {

storage_hasher::storage_hasher(file_storage& storage, const storage_hasher_options& options)
        :storage_(storage)
        , protocol_(options.protocol_version)
        , checksums_(options.checksums)
        , memory_({.min_chunk_size=options.min_chunk_size, .max_memory=options.max_memory})
        , threads_(options.threads)
{
    if (storage.piece_size() == 0)
        storage.set_piece_size(choose_piece_size(storage));

    Expects(protocol_ != dottorrent::protocol::none);
    Expects(std::has_single_bit(storage.piece_size()));

    // allocate the required pieces in storage objects
    if (protocol_ == protocol::v1 || protocol_ == protocol::hybrid) {
        storage.allocate_pieces();
    }
    cumulative_file_size_ = inclusive_file_size_scan(storage);
}

file_storage& storage_hasher::storage()
{
    return storage_.get();
}

const file_storage& storage_hasher::storage() const
{
    return storage_;
}

void storage_hasher::start() {
    if (done())
        throw std::runtime_error("cannot start finished or cancelled hasher");

    auto& storage = storage_.get();
    auto chunk_size = std::max(memory_.min_chunk_size, storage.piece_size());

    if (protocol_ == protocol::v1) {
        reader_ = std::make_unique<v1_chunk_reader>(storage_, chunk_size, memory_.max_memory);
    }
    else {
        reader_ = std::make_unique<v2_chunk_reader>(storage_, chunk_size, memory_.max_memory);
    }

    // add file checksums hashers and register them with the reader

    for (auto algo : checksums_) {
        auto& h = checksum_hashers_.emplace_back(std::make_unique<checksum_hasher>(storage_, algo));
        reader_->register_checksum_queue(h->get_queue());
    }

    // add piece hashers and register them with the reader

    if (protocol_ == protocol::v1) {
        v1_hasher_ = std::make_unique<v1_chunk_hasher>(storage_, threads_);
        reader_->register_hash_queue(v1_hasher_->get_queue());
    }

    if (protocol_ == protocol::v2 || protocol_ == protocol::hybrid) {
        v2_hasher_ = std::make_unique<v2_chunk_hasher>(storage_, threads_);
        reader_->register_hash_queue(v2_hasher_->get_queue());
    }

    // start all parts

    if (v1_hasher_)
        v1_hasher_->start();
    if (v2_hasher_)
        v2_hasher_->start();
    for (auto& ch : checksum_hashers_) {
        ch->start();
    }
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
    if (v1_hasher_) v1_hasher_->request_cancellation();
    if (v2_hasher_) v2_hasher_->request_cancellation();
    for (auto& ch : checksum_hashers_) { ch->request_cancellation(); }

    // wait for all tasks to complete
    reader_->wait();
    if (v1_hasher_) v1_hasher_->wait();
    if (v2_hasher_) v2_hasher_->wait();
    for (auto& ch : checksum_hashers_) { ch->wait(); }

    cancelled_ = true;
    stopped_ = true;
}

void storage_hasher::wait()
{
    if (!started())
        throw std::runtime_error("hasher not running");
    if (done())
        throw std::runtime_error("hasher already done");

    reader_->wait();
    // no pieces will be added after the reader finishes.
    // So we can signal hashers that they can shutdown after
    // finishing all remaining work
    if (v1_hasher_) v1_hasher_->request_stop();
    if (v2_hasher_) v2_hasher_->request_stop();
    for (auto& ch : checksum_hashers_) {
        ch->request_stop();
    }

    if (v1_hasher_) v1_hasher_->wait();
    if (v2_hasher_) v2_hasher_->wait();
    for (auto& ch : checksum_hashers_) {
        ch->wait();
    }

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
    // make sure to return 0 if the hasher was not yet started.
    switch (protocol_) {
    case protocol::v1: {
        if (v1_hasher_ == nullptr) return 0;
        return v1_hasher_->bytes_hashed();
    }
    // protocol::v2 || protocol::hybrid
    default: {
        if (v2_hasher_ == nullptr) return 0;
        return v2_hasher_->bytes_hashed();
    }
    }
}

storage_hasher::file_progress_data storage_hasher::current_file_progress() const noexcept
{
    const auto& storage = storage_.get();
    const auto bytes = bytes_hashed();

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
}
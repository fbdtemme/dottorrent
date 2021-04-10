#include <bit>

#include "dottorrent/storage_verifier.hpp"

#include "dottorrent/v1_chunk_reader.hpp"
#include "dottorrent/v2_chunk_reader.hpp"

#include "dottorrent/v1_chunk_hasher_sb.hpp"
#include "dottorrent/v1_chunk_hasher_mb.hpp"
#include "dottorrent/v2_chunk_hasher_sb.hpp"
#include "dottorrent/v2_chunk_hasher_mb.hpp"
#include "dottorrent/v1_piece_verifier.hpp"
#include "dottorrent/v2_piece_verifier.hpp"


namespace dottorrent {

storage_verifier::storage_verifier(file_storage& storage, const storage_verifier_options& options)
        : storage_(storage)
        , protocol_(options.protocol_version)
        , threads_(options.threads)
        , io_block_size_()
        , queue_capacity_()
        , enable_multi_buffer_hashing_(options.enable_multi_buffer_hashing)
{
    file_storage& st = storage_;

    // auto determine protocol version if not set
    if (protocol_ == protocol::none) {
        protocol_ = st.protocol();
    }
    auto piece_size = storage.piece_size();

#ifdef DOTTORRENT_USE_ISAL
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

    if (protocol_ == protocol::v1) {
        cumulative_file_size_ = inclusive_file_size_scan_v1(storage);
    }
    else {
        cumulative_file_size_ = inclusive_file_size_scan_v2(storage);
    }
}

file_storage& storage_verifier::storage()
{
    return storage_.get();
}

const file_storage& storage_verifier::storage() const
{
    return storage_;
}

enum protocol storage_verifier::protocol() const noexcept
{
    return protocol_;
}


void storage_verifier::start() {
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
#ifdef DOTTORRENT_USE_ISAL
        if (enable_multi_buffer_hashing_)
            hasher_ = std::make_unique<v1_chunk_hasher_mb>(storage_, queue_capacity_, threads_);
        else
            hasher_ = std::make_unique<v1_chunk_hasher_sb>(storage_, queue_capacity_, threads_);
#else
        hasher_ = std::make_unique<v1_chunk_hasher_sb>(storage_, queue_capacity_, threads_);
#endif
        reader_->register_hash_queue(hasher_->get_queue());

        verifier_ = std::make_unique<v1_piece_verifier>(storage_, -1, 1);
        hasher_->register_v1_hashed_piece_queue(verifier_->get_v1_queue());
    }
    else {

#ifdef DOTTORRENT_USE_ISAL
        if (enable_multi_buffer_hashing_)
            hasher_ = std::make_unique<v2_chunk_hasher_mb>(
                    storage_, queue_capacity_, protocol_ == protocol::hybrid, threads_);
        else
            hasher_ = std::make_unique<v2_chunk_hasher_sb>(
                    storage_, queue_capacity_, false, threads_);
#else
        hasher_ = std::make_unique<v2_chunk_hasher_sb>(
                storage_, queue_capacity_, false, threads_);
#endif
        reader_->register_hash_queue(hasher_->get_queue());

        verifier_ = std::make_unique<v2_piece_verifier>(storage_, -1, 1);
        hasher_->register_v1_hashed_piece_queue(verifier_->get_v1_queue());
        hasher_->register_v2_hashed_piece_queue(verifier_->get_v2_queue());
    }

    // start all parts
    verifier_->start();
    hasher_->start();
    reader_->start();
    started_ = true;
}

void storage_verifier::cancel() {
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
    verifier_->request_cancellation();

    // wait for all tasks to complete
    reader_->wait();
    hasher_->wait();
    verifier_->wait();

    cancelled_ = true;
    stopped_ = true;
}

void storage_verifier::wait()
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
    hasher_->wait();

    verifier_->request_stop();
    verifier_->wait();

    stopped_ = true;
}

bool storage_verifier::running() const noexcept
{
    return started_ && !cancelled_ && !stopped_;
}

bool storage_verifier::started() const noexcept
{
    return started_;
}

bool storage_verifier::cancelled() const noexcept
{
    return cancelled_;
}

bool storage_verifier::done() const noexcept
{
    return cancelled_ || (started_ && stopped_);
}

std::size_t storage_verifier::bytes_read() const noexcept
{
    return reader_->bytes_read();
}

std::size_t storage_verifier::bytes_hashed() const noexcept
{
    return hasher_->bytes_hashed();
}

std::size_t storage_verifier::bytes_done() const noexcept
{
    return hasher_->bytes_done();
}


file_progress_data storage_verifier::current_file_progress() const noexcept
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

const std::vector<std::uint8_t>& storage_verifier::result() const noexcept
{
    return verifier_->result();
}

double storage_verifier::percentage(std::size_t file_index) const noexcept
{
    return verifier_->percentage(file_index);
}

double storage_verifier::percentage(const file_entry& entry) const
{
    file_storage storage = storage_;

    std::size_t file_index = std::distance(storage.begin(), rng::find(storage, entry));
    if (file_index == storage.file_count())
        throw std::invalid_argument("entry does not exist in storage");

    return verifier_->percentage(file_index);
}

} //namespace dottorrent
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
#include <unordered_set>

#include <gsl-lite/gsl-lite.hpp>

#include "dottorrent/literals.hpp"
#include "dottorrent/file_entry.hpp"
#include "dottorrent/file_storage.hpp"
#include "dottorrent/file_progress_data.hpp"

#include "dottorrent/data_chunk.hpp"
#include "dottorrent/hash.hpp"
#include "dottorrent/checksum.hpp"
#include "dottorrent/chunk_reader.hpp"
#include "dottorrent/chunk_hasher_single_buffer.hpp"
#include "dottorrent/hashed_piece_processor.hpp"
#include "hashed_piece_verifier.hpp"

namespace dottorrent {

namespace fs = std::filesystem;
using namespace dottorrent::literals;
using namespace std::chrono_literals;


/// Options to control the memory usage of a storage_verifier.
struct storage_verifier_options
{
    /// The bittorrent procol version to verify.
    /// When none the storage verifier the highest protocol available in the metafile.
    protocol protocol_version = protocol::none;
    /// The minimum size of a block to read from disk.
    /// For piece sizes smaller than the min_chunk_size multiple pieces
    /// will be read in a single block for faster disk I/O.
    std::optional<std::size_t> min_io_block_size = std::nullopt;
    /// Max size of all file chunks in memory. The default capacity is determined by
    std::optional<std::size_t> max_memory = std::nullopt;
    /// Weither to enable multi-buffer hashing if linked against Intel ISA-L.
    bool enable_multi_buffer_hashing = true;

    /// Number of threads to hash pieces.
    /// Total number of threads will be equal to:
    /// 1 main thread + 1 reader + <thread> piece hashers + <#checksums types> checksum hashers
    std::size_t threads = 2;
};



class storage_verifier
{
public:
    explicit storage_verifier(
            file_storage& storage,
            const storage_verifier_options& options = {});

    /// Return a reference to the file_storage object.
    file_storage& storage();

    const file_storage& storage() const;

    enum protocol protocol() const noexcept;

    void start();

    void cancel();

    /// Wait for all threads to complete
    void wait();

    bool running() const noexcept;

    bool started() const noexcept;

    bool cancelled() const noexcept;

    /// Check if the hasher is still processing tasks or is just waiting.
    bool done() const noexcept;

    /// The number of bytes read from disk by the reader thread.
    std::size_t bytes_read() const noexcept;

    /// Return the number of bytes hashed by the v1 or v2 hasher,
    /// or the average between the two for hybrid torrents.
    std::size_t bytes_hashed() const noexcept;

    /// Return the number of bytes hashed by the v1 or v2 hasher,
    /// or the average between the two for hybrid torrents.
    std::size_t bytes_done() const noexcept;

    file_progress_data current_file_progress() const noexcept;

    const std::vector<std::uint8_t>& result() const noexcept;

    double percentage(std::size_t file_index) const noexcept;

    double percentage(const file_entry& entry) const;

    ~storage_verifier();

private:
    std::reference_wrapper<file_storage> storage_;
    enum protocol protocol_;

    std::size_t threads_;
    std::size_t io_block_size_;
    std::size_t queue_capacity_;
    bool enable_multi_buffer_hashing_;

    std::unique_ptr<chunk_reader> reader_;
    std::unique_ptr<chunk_processor> hasher_;
    std::unique_ptr<hashed_piece_verifier> verifier_;

    bool started_ = false;
    bool stopped_ = false;
    bool cancelled_ = false;

    // progress info
    mutable std::size_t current_file_index_ = 0;
    std::vector<std::size_t> cumulative_file_size_;
};

} // namespace dottorrent
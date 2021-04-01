#pragma once

#include <cstdint>

namespace dottorrent {

/// Progress information used by storage_hasher and storage_verifier.
struct file_progress_data
{
    /// The index of the current file being hashed
    std::size_t file_index;
    /// The number of bytes of the file that are processed.
    std::size_t bytes_hashed;
};

}
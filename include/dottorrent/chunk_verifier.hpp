#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gsl-lite/gsl-lite.hpp>

#include "dottorrent/chunk_hasher_single_buffer.hpp"

namespace dottorrent {

class chunk_verifier : public chunk_hasher_single_buffer
{
public:
    using chunk_hasher_single_buffer::chunk_hasher_single_buffer;
    // Return for each block weither it is valid or not.
    // For v1 torrents each block is equal to the piece size.
    // For v2 torrents each block is equal to 16 KiB.
    virtual const std::vector<std::uint8_t>& result() const noexcept = 0;

    virtual double percentage(std::size_t file_index) const noexcept = 0;
};

}
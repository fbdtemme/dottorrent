#pragma once

#include "dottorrent/chunk_hasher_single_buffer.hpp"

namespace dottorrent {

class chunk_hasher_multi_buffer : public chunk_processor_base
{
public:
    using chunk_processor_base::chunk_processor_base;

protected:
    void run(std::stop_token stop_token, int thread_idx) override;

    virtual void hash_chunk(std::vector<std::unique_ptr<multi_buffer_hasher>>& hashers, const data_chunk& chunk) = 0;
};

} // namespace dottorrent
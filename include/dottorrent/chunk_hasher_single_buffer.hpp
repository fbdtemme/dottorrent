#pragma once

#include "dottorrent/chunk_processor_base.hpp"

namespace dottorrent {

class chunk_hasher_single_buffer : public chunk_processor_base
{
public:
    using chunk_processor_base::chunk_processor_base;

protected:
    void run(int thread_idx) override;

    virtual void hash_chunk(std::vector<std::unique_ptr<single_buffer_hasher>>& hashers, const data_chunk& chunk) = 0;
};

} // namespace dottorrent
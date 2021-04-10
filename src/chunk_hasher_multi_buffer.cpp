#include "dottorrent/chunk_hasher_multi_buffer.hpp"

namespace dottorrent {

void chunk_hasher_multi_buffer::run(std::stop_token stop_token, int thread_idx)
{
    Expects(stop_token.stop_possible());

    // copy the global hasher object to a per-thread hasher
    std::vector<std::unique_ptr<multi_buffer_hasher>> hashers{};
    for (const auto f : hash_functions_) {
        hashers.push_back(std::move(make_multi_buffer_hasher(f)));
    }

    data_chunk item{};

    // Process tasks until stopped is set
    while (!stop_token.stop_requested() && stop_token.stop_possible()) {
        queue_->pop(item);
        // check if the data_chunk is valid or a stop wake-up signal
        if (item.data == nullptr &&
                item.piece_index == std::numeric_limits<std::uint32_t>::max() &&
                item.file_index == std::numeric_limits<std::uint32_t>::max())
        {
            Ensures(stop_token.stop_requested());
            break;
        }

        hash_chunk(hashers, item);
        item.data.reset();
    }

    // finish pending tasks if the hasher is not cancelled,
    // otherwise discard all remaining work
    if (!cancelled_.load(std::memory_order_relaxed)) {
        while (queue_->try_pop(item)) {
            if (item.data == nullptr &&
                    item.piece_index == std::numeric_limits<std::uint32_t>::max() &&
                    item.file_index == std::numeric_limits<std::uint32_t>::max())
            {
                break;
            }
            hash_chunk(hashers, item);
            item.data.reset();
        }
    }

    done_[thread_idx] = true;
}

}
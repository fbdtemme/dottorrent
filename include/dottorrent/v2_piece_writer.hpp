#pragma once

#include <vector>
#include <atomic>
#include <mutex>
#include <gsl-lite/gsl-lite.hpp>

#include "dottorrent/merkle_tree.hpp"
#include "dottorrent/file_storage.hpp"
#include "dottorrent/hash_function.hpp"

#include "dottorrent/hasher/single_buffer_hasher.hpp"
#include "dottorrent/hasher/multi_buffer_hasher.hpp"
#include "dottorrent/hashed_piece_processor.hpp"
#include "dottorrent/concurrent_queue_processor.hpp"
#include "dottorrent/hashed_piece.hpp"
#include "dottorrent/hasher/factory.hpp"
#include "dottorrent/hash_function.hpp"
#include "dottorrent/utils.hpp"
#include "dottorrent/hashed_piece_processor.hpp"

namespace dottorrent {

/// Contains common code of both v2_chunk_hasher_sb and v2_chunk_hasher_mb.
///
class v2_piece_writer : public hashed_piece_processor
{
public:
    v2_piece_writer(file_storage& storage, std::size_t capacity, bool v1_compatible = false, std::size_t max_concurrency = 1);

    void start() override;

    void request_cancellation() override;

    void request_stop() override;

    void wait() override;

    std::shared_ptr<v1_piece_queue_type> get_v1_queue() override;

    std::shared_ptr<v2_piece_queue_type> get_v2_queue() override;

protected:
    void initialize_trees(const file_storage& storage);


    template <typename HasherType>
    void set_piece_layers_and_root(file_storage& storage, HasherType& hasher, std::size_t file_index);

    void set_finished_piece(const v1_hashed_piece& finished_piece);

    void set_finished_piece(const v2_hashed_piece& finished_piece);

private:
    std::reference_wrapper<file_storage> storage_;
    std::vector<merkle_tree<hash_function::sha256>> merkle_trees_ {};
    /// Vector with the count of 16 KiB blocks_hashed per file
    std::vector<std::atomic<std::size_t>> file_blocks_hashed_ {};
    bool add_v1_compatibility_ = false;

    concurrent_queue_processor<v1_hashed_piece> v1_processor_;
    concurrent_queue_processor<v2_hashed_piece> v2_processor_;
};


} // namespace dottorrent
#pragma once

#include <vector>
#include <atomic>
#include <mutex>

#include "dottorrent/merkle_tree.hpp"
#include "dottorrent/file_storage.hpp"
#include "dottorrent/hash_function.hpp"

#include "dottorrent/hasher/single_buffer_hasher.hpp"
#include "dottorrent/hasher/multi_buffer_hasher.hpp"

namespace dottorrent {

/// Contains common code of both v2_chunk_hasher_sb and v2_chunk_hasher_mb.
///
class v2_chunk_hasher_mixin
{

protected:
    void initialize_trees_and_v1_offsets(const file_storage& storage);

    template <typename HasherType>
    void set_piece_layers_and_root(file_storage& storage, HasherType& hasher, std::size_t file_index);

    void process_piece_hash(file_storage& storage, std::size_t piece_idx,
                            std::size_t file_index, const sha1_hash& piece_hash);




    std::vector<merkle_tree<hash_function::sha256>> merkle_trees_ {};
    /// Vector with the count of hashed bytes per file
    std::vector<std::atomic<std::size_t>> file_bytes_hashed_ {};

    bool add_v1_compatibility_ = false;
    // for each file the v1 piece index the file starts at.
    std::vector<std::size_t> v1_piece_offsets_ {};
    std::vector<std::byte> padding_ {};
    std::mutex padding_mutex_ {};
};

extern template void v2_chunk_hasher_mixin::set_piece_layers_and_root<>(
        file_storage&, single_buffer_hasher&, std::size_t);

#if defined (DOTTORRENT_USE_ISAL)
extern template void v2_chunk_hasher_mixin::set_piece_layers_and_root<>(
        file_storage&, multi_buffer_hasher&, std::size_t);
#endif



} // namespace dottorrent
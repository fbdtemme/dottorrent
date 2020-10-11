#pragma once
#include <span>
#include <vector>
#include <mutex>
#include <atomic>

#include <gsl/gsl_assert>

#include "dottorrent/merkle_tree.hpp"
#include "dottorrent/chunk_hasher.hpp"

namespace dottorrent {

class v2_chunk_hasher : public chunk_hasher
{
public:
    using base_type = chunk_hasher;


    explicit v2_chunk_hasher(file_storage& storage, std::size_t thread_count = 1);

protected:
    void hash_chunk(hasher& hasher, const data_chunk& chunk) final;

    void set_piece_layers_and_root(hasher& hasher, std::size_t file_index);

private:
    std::vector<merkle_tree<hash_function::sha256>> merkle_trees_;
    /// Vector with the count of hashed bytes per file.per
    std::vector<std::atomic<std::size_t>> file_bytes_hashed_ {};
    std::size_t piece_size_;
};

} // namespace dottorrent

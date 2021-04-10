#pragma once

#include "dottorrent/hashed_piece.hpp"
#include "dottorrent/hashed_piece_verifier.hpp"
#include "dottorrent/concurrent_queue_processor.hpp"
#include "dottorrent/file_storage.hpp"

namespace dottorrent
{

class v2_piece_verifier : public hashed_piece_verifier
{
public:
    v2_piece_verifier(file_storage& storage, std::size_t capacity, std::size_t max_concurrency = 1);

    void start() override;

    void request_cancellation() override;

    void request_stop() override;

    void wait() override;

    std::shared_ptr<v1_piece_queue_type> get_v1_queue() override;

    std::shared_ptr<v2_piece_queue_type> get_v2_queue() override;

    const std::vector<std::uint8_t>& result() const noexcept override;

    double percentage(std::size_t file_index) const noexcept override;

protected:
    void initialize_offsets_and_trees();

    void verify_finished_piece(const v2_hashed_piece& finished_piece);
    template <typename HasherType>
    void verify_piece_layers_and_root(HasherType& hasher, std::size_t file_index);

private:
    std::reference_wrapper<file_storage> storage_;
    std::vector<merkle_tree<hash_function::sha256>> merkle_trees_ {};
    /// Vector with the count of 16 KiB blocks_hashed per file
    std::vector<std::atomic<std::size_t>> file_blocks_hashed_ {};

    std::vector<std::uint8_t> piece_map_;
    std::vector<std::size_t> file_offsets_;

    concurrent_queue_processor<v2_hashed_piece> processor_;
};

}
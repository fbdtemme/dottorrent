#include "dottorrent/v2_chunk_reader.hpp"

#include <fstream>

namespace dottorrent {

void v2_chunk_reader::run()
{
    const auto file_paths = absolute_file_paths(storage_);
    // index of the first piece in a chunk
    std::size_t piece_index = 0;
    // size of the current chunk, less then chunk_size_ for files smaller then `chunk_size_`
    std::size_t current_chunk_size = 0;
    // index of the current file being read in the storage object
    std::size_t file_index = 0;
    // the current file being read, disable read buffer
    std::ifstream f;

    auto& storage = storage_.get();
    const auto piece_size = storage.piece_size();
    const auto pieces_per_chunk = chunk_size_ / piece_size;
    auto chunk = pool_.get();
    chunk->resize(chunk_size_);

    for (const fs::path& file: file_paths) {
        // set last modified date in the file entry of the storage
        storage.set_last_modified_time(file_index, fs::last_write_time(file));
        f.open(file, std::ios::binary);
        // reset piece index. piece index is per file for v2!
        piece_index = 0;

        while (!cancelled_.load(std::memory_order_relaxed)) {
            f.read(reinterpret_cast<char*>(chunk->data()), chunk_size_);
            current_chunk_size = f.gcount();
            Expects(current_chunk_size > 0);

            chunk->resize(current_chunk_size);
            bytes_read_.fetch_add(current_chunk_size, std::memory_order_relaxed);

            // push chunk to consumers
            push({static_cast<std::uint32_t>(piece_index),
                  static_cast<std::uint32_t>(file_index),
                  std::move(chunk)});
            // recycle a new chunk from the pool
            chunk = pool_.get();
            chunk->resize(chunk_size_);
            // clear chunk and file offsets
            // increment chunk index
            piece_index += pieces_per_chunk;

            if (f.eof()) break;
        }
        f.close();
        f.clear();
        ++file_index;
    }
}

void v2_chunk_reader::push(const data_chunk& chunk) {
    Expects(chunk.data);
    Expects(0 <= chunk.piece_index && chunk.piece_index < storage_.get().piece_count());

    // hasher queues
    for (auto& queue : hash_queues_) {
        queue->push(chunk);
    }
    for (auto& queue: checksum_queues_) {
        queue->push(chunk);
    }
}

}
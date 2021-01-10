#include "dottorrent/v1_chunk_reader.hpp"

#include <fstream>

namespace dottorrent {

void v1_chunk_reader::run()
{
    const auto file_paths = absolute_file_paths(storage_);
    // index of the first piece in a chunk
    std::size_t piece_index = 0;
    // position of the first free byte in the current chunk
    std::size_t chunk_offset = 0;
    // position of the first byte from a different file than the
    // previous byte in the current chunk. A file offset of zero
    // indicates all data is from a single file.
    std::size_t offset_index = 0;
    // vector of all offsets in the current chunk.
    std::vector<std::size_t> file_offsets {};
    // index of the current file being read in the storage object
    std::size_t file_index = 0;
    // the current file being read, disable read buffer
    std::ifstream f;

    auto& storage = storage_.get();
    const auto piece_size = storage.piece_size();

    Expects(chunk_size_ % piece_size == 0);
    const auto pieces_per_chunk = chunk_size_ / piece_size;
    auto chunk = pool_.get();
    chunk->resize(chunk_size_);

    for (const fs::path& file: file_paths) {

        // handle pieces if the file does not exists. Used when verifying torrents.
        if (!fs::exists(file)) {
            auto missing_file_size = storage.at(file_index).file_size();

            // fill current piece in this chunk with zero bytes.
            if (chunk_offset != 0) {
                auto pieces_in_this_chunk = (chunk_offset + piece_size - 1) / piece_size;
                auto missing_piece_bytes = (pieces_in_this_chunk * piece_size) - chunk_offset;
                // add a offset to indicate that the piece contains zero'd bytes from a new file
                file_offsets.push_back(chunk_offset);
                std::fill_n(std::next(chunk->data(), chunk_offset), missing_piece_bytes, std::byte(0));
                // we processed missing_piece_bytes bytes from the total missing_file_size.
                // resize chunk to the number of pieces it contains
                chunk_offset += missing_piece_bytes;
                missing_file_size -= missing_piece_bytes;
                chunk->resize(chunk_offset);

                // push the chunk with partially zero'd data.
                push({static_cast<std::uint32_t>(piece_index),
                      static_cast<std::uint32_t>(file_index - file_offsets.size()),
                      std::move(chunk)
                });
                // recycle a new chunk from the pool
                chunk = pool_.get();
                chunk->resize(chunk_size_);
                // clear chunk and file offsets
                chunk_offset = 0;
                file_offsets.clear();
                // increment chunk index
                piece_index += pieces_in_this_chunk;
            }

            // advance piece index to the first piece that contains data from a new file.
            // push empty pieces to make hashers aware that these bytes are done,
            // but they do not require any work
            auto first_new_piece_index = piece_index + missing_file_size / piece_size;
            for ( ; piece_index < first_new_piece_index; ++piece_index)
            {
                push({static_cast<std::uint32_t>(piece_index),
                      static_cast<std::uint32_t>(file_index),
                      nullptr});
            }

            // Set the bytes of the last piece of the missing file to zero.
            auto last_piece_bytes = missing_file_size % piece_size;
            std::fill_n(chunk->data() + chunk_offset, last_piece_bytes, std::byte(0));
            chunk_offset += last_piece_bytes;

            // open the next file and continue hashing
            continue;
        }

        // set last modified date in the file entry of the storage
        storage.set_last_modified_time(file_index, fs::last_write_time(file));
        f.open(file, std::ios::binary);
        // increment file index, file_index points to the next file now
        ++file_index;

        while (!cancelled_.load(std::memory_order_relaxed)) {
            f.read(reinterpret_cast<char*>(std::next(chunk->data(), chunk_offset)),
                    (chunk_size_ - chunk_offset)
            );
            file_offsets.push_back(chunk_offset);
            chunk_offset += f.gcount();
            bytes_read_.fetch_add(f.gcount(), std::memory_order_relaxed);

            if (chunk_offset == chunk_size_) {
                // push chunk to consumers
                push({static_cast<std::uint32_t>(piece_index),
                      static_cast<std::uint32_t>(file_index-file_offsets.size()),
                      chunk
                });
                // recycle a new chunk from the pool
                chunk = pool_.get();
                chunk->resize(chunk_size_);
                // clear chunk and file offsets
                chunk_offset = 0;
                file_offsets.clear();
                // increment chunk index
                piece_index += pieces_per_chunk;
            }
            // eof reached
            if (f.eof()) break;
        }
        f.close();
        f.clear();
    }

    // update size for last piece
    if (chunk_offset > 0) {
        chunk->resize(chunk_offset);
        push({static_cast<std::uint32_t>(piece_index),
              static_cast<std::uint32_t>(file_index-file_offsets.size()),
              std::move(chunk)
        });
    }
}

void v1_chunk_reader::push(const data_chunk& chunk) {
    for (auto& queue : hash_queues_) {
        queue->push(chunk);
    }
    for (auto& queue: checksum_queues_) {
        queue->push(chunk);
    }
}

} // namespace dottorrent
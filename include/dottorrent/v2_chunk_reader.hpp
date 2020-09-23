#pragma once
#include "dottorrent/chunk_reader.hpp"

namespace dottorrent {

class v2_chunk_reader : public chunk_reader
{
public:
    using chunk_reader::chunk_reader;

    void run() final;

private:
    /// @param file_idx: index of the file the data is read from,
    ///     when a chunk consists of data from multiple files,
    ///     the index of the first part as given by file_offset.
    void push(const data_chunk& chunk);
};

}
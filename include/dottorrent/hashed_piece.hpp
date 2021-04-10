#pragma once

#include "hash.hpp"

namespace dottorrent {

struct v1_hashed_piece
{
    sha1_hash hash;
    std::size_t index;
};

struct v2_hashed_piece
{
    sha256_hash hash;
    std::size_t file_index;
    std::size_t leaf_index;
};

}
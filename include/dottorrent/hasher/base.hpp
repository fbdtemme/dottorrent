#pragma once
#include <span>
#include <cstddef>

//#include "dottorrent/hash_function.hpp"


namespace dottorrent {

class hasher
{
public:
    virtual void update(std::span<const std::byte> data) = 0;

    virtual void finalize_to(std::span<std::byte> out) = 0;

    virtual ~hasher() = default;
};

}
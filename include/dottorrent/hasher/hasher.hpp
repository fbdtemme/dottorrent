#pragma once
#include <span>
#include <cstddef>


namespace dottorrent {

class hasher
{
public:
    virtual void update(std::span<const std::byte> data) = 0;

    virtual void update(std::string_view data)
    {
        auto s = std::span{reinterpret_cast<const std::byte*>(data.data()), data.size()};
        update(s);
    }

    virtual void finalize_to(std::span<std::byte> out) = 0;

    virtual ~hasher() = default;
};
}
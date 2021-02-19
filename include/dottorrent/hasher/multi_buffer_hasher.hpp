#pragma once
#include <vector>
#include <span>
#include <string_view>
#include <cstring>

#include <gsl-lite/gsl-lite.hpp>

class multi_buffer_hasher
{
public:
    virtual void submit(std::size_t job_id, std::span<const std::byte> data) = 0;

    virtual void submit(std::size_t job_id, std::string_view data)
    {
        auto s = std::span{reinterpret_cast<const std::byte*>(data.data()), data.size()};
        return submit(job_id, s);
    }

    virtual std::size_t submit(std::span<const std::byte> data) = 0;

    virtual std::size_t submit(std::string_view data)
    {
        auto s = std::span{reinterpret_cast<const std::byte*>(data.data()), data.size()};
        return submit(s);
    }

    virtual void submit_first(std::size_t job_idx, std::span<const std::byte> data) = 0;

    virtual std::size_t submit_first(std::span<const std::byte> data) = 0;

    virtual std::size_t submit_first(std::string_view data)
    {
        auto s = std::span{reinterpret_cast<const std::byte*>(data.data()), data.size()};
        return submit_first(s);
    }

    virtual void submit_update(std::size_t job_id, std::span<const std::byte> data) = 0;

    virtual void submit_update(std::size_t job_id, std::string_view data)
    {
        auto s = std::span{reinterpret_cast<const std::byte*>(data.data()), data.size()};
        submit_update(job_id, s);
    }

    virtual void submit_last(std::size_t job_id, std::span<const std::byte> data) = 0;

    virtual void submit_last(std::size_t job_id, std::string_view data)
    {
        auto s = std::span{reinterpret_cast<const std::byte*>(data.data()), data.size()};
        submit_last(job_id, s);
    }

    virtual std::size_t job_count() = 0;

    virtual void finalize_to(std::size_t job_id, std::span<std::byte> buffer) = 0;

    virtual void reset() = 0;

    virtual void resize(std::size_t len) = 0;

    virtual ~multi_buffer_hasher() = default;
};
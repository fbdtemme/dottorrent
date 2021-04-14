#pragma once

#include <optional>
#include <memory>

#include "dottorrent/concurrent_queue.hpp"
#include "dottorrent/hashed_piece.hpp"

namespace dottorrent {

class hashed_piece_processor
{
public:
    using v1_piece_queue_type = concurrent_queue<std::optional<v1_hashed_piece>>;
    using v2_piece_queue_type = concurrent_queue<std::optional<v2_hashed_piece>>;

    virtual void start() = 0;

    virtual void request_cancellation() = 0;

    virtual void request_stop() = 0;

    virtual void wait() = 0;

    virtual bool running() const noexcept = 0;

    /// Return true if the thread has been started.
    virtual bool started() const noexcept = 0;

    virtual bool cancelled() const noexcept = 0;

    /// Check if the hasher has completed all work or is cancelled.
    virtual bool done() const noexcept = 0;

    virtual std::shared_ptr<v1_piece_queue_type> get_v1_queue()
    { return nullptr; };

    virtual std::shared_ptr<v2_piece_queue_type> get_v2_queue()
    { return nullptr; };

    virtual ~hashed_piece_processor() = default;
};

}
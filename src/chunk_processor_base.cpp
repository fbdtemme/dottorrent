#include "dottorrent/chunk_processor_base.hpp"

namespace dottorrent {

chunk_processor_base::chunk_processor_base(file_storage& storage, std::vector<hash_function> hf, std::size_t capacity,
        std::size_t thread_count)
        : storage_(storage)
        , threads_(thread_count)
        , queue_(std::make_shared<work_queue_type>(capacity))
        , hash_functions_(std::move(hf))
        , done_(thread_count)
{
    Expects(thread_count > 0);
}

void chunk_processor_base::start() {
    Ensures(!started());
    Ensures(!cancelled());

    for (std::size_t i = 0; i < threads_.size(); ++i) {
        done_[i] = false;
        threads_[i] = std::jthread([=, this](std::stop_token st) { run(std::move(st), i); });
    }
    started_.store(true, std::memory_order_release);
}

void chunk_processor_base::request_stop() {
    rng::for_each(threads_, [](std::jthread& t) { t.request_stop(); });
}

void chunk_processor_base::request_cancellation() {
    cancelled_.store(true, std::memory_order_relaxed);
    request_stop();
}

void chunk_processor_base::wait() {
    if (!started()) return;

    for (auto i = 0; i < threads_.size(); ++i) {
        queue_->push({
                std::numeric_limits<std::uint32_t>::max(),
                std::numeric_limits<std::uint32_t>::max(),
                nullptr
        });
    }

    for  (auto i = 0; !done() && i < threads_.size(); ++i) {
        queue_->push({
                std::numeric_limits<std::uint32_t>::max(),
                std::numeric_limits<std::uint32_t>::max(),
                nullptr
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // should all be finished by now and ready to join
    for (auto& t : threads_) {
        t.join();
    }
    Ensures(std::all_of(done_.begin(), done_.end(),
            [](auto& b) { return b.load(std::memory_order_relaxed); }));
}

bool chunk_processor_base::running() const noexcept {
    return started_.load(std::memory_order_relaxed) &&
            !cancelled_.load(std::memory_order_relaxed) &&
            !stopped_.load(std::memory_order_relaxed);
}

auto chunk_processor_base::started() const noexcept -> bool
{ return started_.load(std::memory_order_relaxed); }

auto chunk_processor_base::cancelled() const noexcept -> bool
{ return cancelled_.load(std::memory_order_relaxed); }

auto chunk_processor_base::done() const noexcept -> bool {
    bool all_done = std::all_of(done_.begin(), done_.end(),
            [](auto& b) { return b.load(std::memory_order_relaxed); });
    return cancelled() || (started() && all_done);
}

std::shared_ptr<typename chunk_processor_base::work_queue_type> chunk_processor_base::get_queue()
{ return queue_; }

std::shared_ptr<const typename chunk_processor_base::work_queue_type> chunk_processor_base::get_queue() const
{ return queue_; }

auto chunk_processor_base::bytes_hashed() const noexcept -> std::size_t
{ return bytes_hashed_.load(std::memory_order_relaxed); }

auto chunk_processor_base::bytes_done() const noexcept -> std::size_t
{ return bytes_done_.load(std::memory_order_relaxed); }

void chunk_processor_base::register_v1_hashed_piece_queue(const std::shared_ptr<v1_hashed_piece_queue>& queue)
{ v1_hashed_piece_queue_ = queue; }

void chunk_processor_base::register_v2_hashed_piece_queue(const std::shared_ptr<v2_hashed_piece_queue>& queue)
{ v2_hashed_piece_queue_ = queue; }

chunk_processor_base::~chunk_processor_base()
{
    if (started() && !done()) {
        request_stop();
        wait();
    }
    // Destructor should only be called after all workers are done
    Ensures(std::all_of(done_.begin(), done_.end(),
                        [](auto& b) { return b.load(std::memory_order_relaxed); }));
};

}
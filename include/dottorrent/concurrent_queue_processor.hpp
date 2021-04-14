#pragma once

#include <thread>
#include <functional>
#include <ranges>

#include "concurrent_queue.hpp"

namespace dottorrent {

namespace rng = std::ranges;


/// A thread pool executing a single function.
template <typename T>
class concurrent_queue_processor
{
public:
    using parameter_type = T;
    using queue_type = concurrent_queue<std::optional<parameter_type>>;

    explicit concurrent_queue_processor(std::size_t capacity)
            : threads_()
            , work_function_()
            , queue_(std::make_shared<queue_type>(capacity))
    {}

    void set_work_function(std::function<void(T&&)> fn)
    {
        work_function_ = fn;
    }

    void set_max_concurrency( std::size_t max_concurrency = 0)
    {
        if (max_concurrency == 0) {
            max_concurrency = std::thread::hardware_concurrency();
        }
        threads_.resize(max_concurrency);
        done_ = std::vector<std::atomic<bool>>(max_concurrency);
    }


    /// Start the worker threads
    void start()
    {
        Ensures(!started());
        Ensures(!cancelled());

        for (std::size_t i = 0; i < threads_.size(); ++i) {
            done_[i] = false;
            threads_[i] = std::jthread([=, this](std::stop_token st) { run(std::move(st), i); });
        }
        started_.store(true, std::memory_order_release);
    }

    /// Signal the workers to shut down after finishing all pending work.
    /// No items can be added to the work queue after this call.
    void request_stop()
    {
        rng::for_each(threads_, [](std::jthread& t) { t.request_stop(); });
    }

    /// Signal the workers to shut down and discard pending work.
    void request_cancellation()
    {
        cancelled_.store(true, std::memory_order_relaxed);
        request_stop();
    }


    /// Block until all workers finish their execution.
    /// Note that workers will only finish after a call to stop or cancel.
    void wait()
    {
        if (!started()) return;

        // Wake threads that remain blocked on pop calls.
        for  (auto i = 0; i < threads_.size(); ++i) {
            queue_->push(std::nullopt);
        }

        // Wake threads that remain blocked on pop calls.
        for  (auto i = 0; !done() && i < threads_.size(); ++i) {
            queue_->try_push(std::nullopt);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // should all be finished by now and ready to join
        for (auto& t : threads_) {
            t.join();
        }
        Expects(done());
    }

    bool running() const noexcept
    {
        return started_.load(std::memory_order_relaxed) &&
                !cancelled_.load(std::memory_order_relaxed) &&
                !stopped_.load(std::memory_order_relaxed);
    }

    /// Return true if the thread has been started.
    bool started() const noexcept
    { return started_.load(std::memory_order_relaxed); }

    bool cancelled() const noexcept
    { return cancelled_.load(std::memory_order_relaxed); }

    /// Check if the hasher has completed all work or is cancelled.
    bool done() const noexcept
    {
        bool all_done = std::all_of(done_.begin(), done_.end(),
                [](auto& b) { return b.load(std::memory_order_relaxed); });
        return cancelled() || (started() && all_done);
    }

    std::shared_ptr<queue_type> get_queue()
    { return queue_; }

    std::shared_ptr<const queue_type> get_queue() const
    { return queue_; }

    ~concurrent_queue_processor()
    {
        if (started() && !done()) {
            request_stop();
            wait();
        }
    }

private:
    void run(std::stop_token stop_token, int thread_idx)
    {
        Ensures(stop_token.stop_possible());
        Ensures(done_[thread_idx] == false);

        std::optional<parameter_type> item {};

        // Process tasks until stopped is set
        while (!stop_token.stop_requested() && stop_token.stop_possible()) {
            queue_->pop(item);
            // check if the data_chunk is valid or a stop wake-up signal, a default constructed parameter type
            if (!item.has_value())
                continue;
            std::invoke(work_function_, std::move(*item));
        }

        // finish pending tasks if the hasher is not cancelled,
        // otherwise discard all remaining work
        if (!cancelled_.load(std::memory_order_relaxed)) {
            while (queue_->try_pop(item)) {
                if (!item.has_value())
                    break;
                std::invoke(work_function_, std::move(*item));
            }
        }
        done_[thread_idx] = true;
        Expects(queue_->size() == 0);
    }

    std::vector<std::jthread> threads_;
    std::function<void(parameter_type&&)> work_function_;
    std::shared_ptr<queue_type> queue_;
    std::atomic<bool> started_ = false;
    std::atomic<bool> cancelled_ = false;
    std::atomic<bool> stopped_ = false;
    std::vector<std::atomic<bool>> done_;
};

} // namespace dottorrent
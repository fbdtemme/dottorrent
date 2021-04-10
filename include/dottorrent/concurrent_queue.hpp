#pragma once

#include <atomic>
#include <cassert>
#include <cstddef> // offsetof
#include <limits>
#include <memory>
#include <new> // std::hardware_destructive_interference_size
#include <stdexcept>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <deque>

#include <atomic>
#include <cassert>
#include <cstddef> // offsetof
#include <limits>
#include <memory>
#include <new> // std::hardware_destructive_interference_size
#include <stdexcept>

namespace dottorrent {

template<typename T, typename Allocator = std::allocator<T>>
class concurrent_queue {
public:
    concurrent_queue(size_t capacity)
            : capacity_(capacity) {}

    concurrent_queue(const concurrent_queue &) = delete;
    concurrent_queue(concurrent_queue &&) = delete;
    concurrent_queue &operator = (const concurrent_queue &) = delete;
    concurrent_queue &operator = (concurrent_queue &&) = delete;

    void push(const T& item) {
        {
            std::unique_lock<std::mutex> lk(mutex_);
            not_full_.wait(lk, [this]() { return queue_.size() < capacity_; });
            queue_.push(item);
        }
        not_empty_.notify_all();
    }

    void push(T&& item) {
        {
            std::unique_lock<std::mutex> lk(mutex_);
            not_full_.wait(lk, [this]() { return queue_.size() < capacity_; });
            queue_.push(std::move(item));
        }
        not_empty_.notify_all();
    }

    bool try_push(T &&item) {
        {
            std::unique_lock<std::mutex> lk(mutex_);
            if (queue_.size() == capacity_)
                return false;
            queue_.push(std::move(item));
        }
        not_empty_.notify_all();
        return true;
    }

    void pop(T &item) {
        {
            std::unique_lock<std::mutex> lk(mutex_);
            not_empty_.wait(lk, [this]() { return !queue_.empty(); });
            item = std::move(queue_.front());
            queue_.pop();
        }
        not_full_.notify_all();
    }

    bool try_pop(T &item) {
        {
            std::unique_lock<std::mutex> lk(mutex_);
            if (queue_.empty())
                return false;
            item = std::move(queue_.front());
            queue_.pop();
        }
        not_full_.notify_all();
        return true;
    }

    std::size_t size() const noexcept
    {
        std::unique_lock<std::mutex> lk(mutex_);
        return queue_.size();
    }

    void set_capacity(std::size_t n) noexcept
    {
        std::unique_lock<std::mutex> lk(mutex_);
        capacity_ = n;
    }

private:
    mutable std::mutex mutex_ {};
    std::condition_variable not_empty_ {};
    std::condition_variable not_full_ {};
    size_t capacity_;
    std::queue<T, std::deque<T, Allocator>> queue_ {};
};


} // namespace dottorrent
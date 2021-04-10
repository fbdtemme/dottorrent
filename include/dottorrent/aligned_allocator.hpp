#pragma once

#include <cstdint>
#include <vector>
#include <iostream>

namespace dottorrent {

/// Allocator for aligned data.
template <typename T, std::size_t Alignment>
class aligned_allocator
{
public:

    // The following will be the same for virtually all allocators.
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T&;
    using const_reference = const T&;
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = ptrdiff_t;

    // Default constructor, copy constructor, rebinding constructor, and destructor.
    // Empty for stateless allocators.
    aligned_allocator() = default;

    aligned_allocator(const aligned_allocator&) = default;

    template <typename U> aligned_allocator(const aligned_allocator<U, Alignment>&) {};

    ~aligned_allocator() = default;

    aligned_allocator& operator=(const aligned_allocator&) = delete;

    // Returns true if and only if storage allocated from *this
    // can be deallocated from other, and vice versa.
    // Always returns true for stateless allocators.
    bool operator==(const aligned_allocator& other) const
    {
        return true;
    }

    bool operator!=(const aligned_allocator& other) const
    {
        return !(*this == other);
    }

    T * address(T& r) const
    {
        return std::addressof(r);
    }

    const T * address(const T& s) const
    {
        return std::addressof(s);
    }

    std::size_t max_size() const
    {
        // The following has been carefully written to be independent of
        // the definition of size_t and to avoid signed/unsigned warnings.
        return (static_cast<std::size_t>(0) - static_cast<std::size_t>(1)) / sizeof(T);
    }

    // The following must be the same for all allocators.
    template <typename U>
    struct rebind
    {
        typedef aligned_allocator<U, Alignment> other;
    } ;

    void construct(T * const p, const T& t) const
    {
        void * const pv = static_cast<void *>(p);

        new (pv) T(t);
    }

    void destroy(T * const p) const
    {
        p->~T();
    }

    // The following will be different for each allocator.
    T * allocate(const std::size_t n) const
    {
        // The return value of allocate(0) is unspecified.
        // Mallocator returns NULL in order to avoid depending
        // on malloc(0)'s implementation-defined behavior
        // (the implementation can define malloc(0) to return NULL,
        // in which case the bad_alloc check below would fire).
        // All allocators can return NULL in this case.
        if (n == 0) {
            return nullptr;
        }

        // All allocators should contain an integer overflow check.
        // The Standardization Committee recommends that std::length_error
        // be thrown in the case of integer overflow.
        if (n > max_size())
        {
            throw std::length_error("aligned_allocator<T>::allocate() - Integer overflow.");
        }

        void * const pv = std::aligned_alloc(Alignment, n * sizeof(T));

        // Allocators should throw std::bad_alloc in the case of memory allocation failure.
        if (pv == nullptr)
        {
            throw std::bad_alloc();
        }

        return static_cast<T *>(pv);
    }

    void deallocate(T * const p, const std::size_t n) const
    {
        std::free(p);
    }

    // The following will be the same for all allocators that ignore hints.
    template <typename U>
    T * allocate(const std::size_t n, const U * /* const hint */) const
    {
        return allocate(n);
    }
};

} // namespace dottorrent
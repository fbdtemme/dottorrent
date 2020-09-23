#pragma once

#include <gsl/gsl>
#include <tbb/concurrent_vector.h>
#include "dottorrent/hash.hpp"
#include "dottorrent/hash_traits.hpp"


namespace dottorrent {

//template <hash_type T> class merkle_tree;

struct merkle_node_position
{
    std::uint32_t layer;
    std::uint32_t index;
};


template <hash_type T>
class merkle_tree
{
public:
    using value_type = T;
    using hasher_type = typename hash_function_traits<T::algorithm>::hasher_type;
    using flat_index = std::size_t;
    using node_position = merkle_node_position;

    explicit merkle_tree() noexcept = default;
    explicit merkle_tree(std::size_t leaf_nodes)
            : data_()
    {
        set_leaf_nodes(leaf_nodes);
    }

    explicit merkle_tree(std::size_t leaf_nodes, const value_type& value)
            : data_()
    {
        set_leaf_nodes(leaf_nodes, value);
    }

    merkle_tree(const merkle_tree& other)
        : data_(other.data_)
    {}

    merkle_tree(merkle_tree&& other) noexcept
            : data_(std::move(other.data_))
    {}

    merkle_tree& operator=(const merkle_tree& other)
    {
        if (&other == this)
            return *this;

        data_ = other.data_;
    }

    merkle_tree& operator=(merkle_tree&& other) noexcept
    {
        if (&other == this)
            return *this;

        data_ = std::move(other.data_);
    }

    /// Set the number of leaf nodes and initialize then to `value`
    /// If no value is given the nodes will be default constructed.
    /// All current values are invalidated.
    void set_leaf_nodes(std::size_t leaf_nodes, const value_type& value = {})
    {
        std::size_t height = detail::log2_ceil(leaf_nodes);
        std::size_t node_count = total_node_count_for_height(height);

        data_.clear();
        data_.assign(node_count, value);
    }

    /// Return true if the merkle root is set.
    const value_type& has_root()
    {
        return data_[0] != value_type{};
    }

    /// Calculate the root and inner hashes from the leaf hashes.
    void update(hasher_type& hasher)
    {

        for (std::size_t layer = tree_height(); layer > 0; --layer) {
            for (std::size_t i = 0; i < nodes_in_layer(layer); ++i) {
                hasher.update(get_node(layer, i));
                hasher.update(get_node(layer, ++i));
                set_node(layer-1, parent(i), hasher.finalize());
            }
        }
    }

    /// Calculate the root and inner hashes from the leaf hashes.
    void update()
    {
        hasher_type h{};
        update(h);
    }

    /// Return the merkle root.
    /// If the merkle root is not set it will return a zero-initialized value.
    const value_type& root()
    { return data_[0]; }

    /// Return a const reference to the underlying representation.
    const std::vector<value_type>& data() const
    { return data_; }

    /// Return a span of pieces in given layer of a subtree starting at the flat `root_index`
    /// Not thread safe
    std::span<const value_type> get_layer(std::size_t depth, std::size_t root_idx = 0)
    {
        Ensures(depth <= tree_height());
        auto begin = std::next(data_.cbegin(), get_flat_index(depth, root_idx));
        return std::span<const value_type>(&*begin, nodes_in_layer(depth));
    }

    /// Return the node at given layer and index
     const value_type& get_node(std::size_t layer, std::size_t index) const
    {
        Ensures(layer <=tree_height());
        Ensures(index < nodes_in_layer(layer));
        return data_[get_flat_index(layer, index)];
    }

    const value_type& get_leaf(std::size_t index) const
    {
        return get_node(tree_height(), index);
    }

    void set_leaf(std::size_t index, const value_type& value)
    {
        set_node(tree_height(), index, value);
    };

    std::size_t node_count() const
    {
        return data_.size();
    }

    std::size_t leaf_count() const
    {
        return (data_.size() + 1) / 2 ;
    }

    std::size_t tree_height() const
    {
        return get_node_layer(data_.size()-1);
    }

private:
    void set_node(std::size_t layer, std::size_t index, const value_type& value) noexcept
    {
        Ensures(layer <= tree_height());
        Ensures(index < nodes_in_layer(layer));
        data_[get_flat_index(layer, index)] = value;
    }

    static constexpr std::size_t nodes_in_layer(std::size_t layer) noexcept
    { return (1U << layer); }

    static constexpr std::size_t total_node_count_for_height(std::size_t height)
    { return (1U << (height + 1)) - 1; }

    static constexpr std::size_t get_flat_index(std::size_t layer, std::size_t index) noexcept
    { return (1U << layer) - 1 + index; }

    static constexpr std::size_t get_node_layer(std::size_t flat_index) noexcept
    { return detail::log2_floor(flat_index+1); }

    static constexpr merkle_node_position get_node_index(std::size_t flat_index) noexcept
    {
        auto layer = get_node_layer(flat_index);
        auto index = flat_index - get_flat_index(layer, 0);
        return {static_cast<std::uint32_t>(layer), static_cast<std::uint32_t>(index)};
    }

    static constexpr std::size_t parent(std::size_t flat_idx) noexcept
    { return (flat_idx - 1) / 2; }

    static constexpr std::size_t left_child(std::size_t flat_idx) noexcept
    { return (flat_idx * 2) + 1; }

    static constexpr std::size_t right_child(std::size_t flat_idx) noexcept
    { return (flat_idx * 2) + 2; }

    /// Return the index of the leftmost leaf of a subtree with root at `flat_idx` and height `n`.
    static constexpr std::size_t left_child_n(std::size_t flat_idx, std::size_t n) noexcept
    { return (flat_idx * (2U << n)) + (2U << n) -1; }

    /// Return the index of the rightmost leaf of a subtree with root at `flat_idx` and height `n`.
    static constexpr std::size_t right_child_n(std::size_t flat_idx, std::size_t n) noexcept
    { return (flat_idx * (2U << n)) + (4U << n) - 2; }

    tbb::concurrent_vector<value_type> data_;
};

} // namespace dottorrent

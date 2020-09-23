#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <filesystem>
#include <type_traits>
#include <cmath>
#include <numeric>
#include <compare>

#include <gsl/gsl>

#include "dottorrent/general.hpp"
#include "dottorrent/bitmask_operators.hpp"
#include "dottorrent/file_entry.hpp"
#include "dottorrent/hash.hpp"
#include "dottorrent/literals.hpp"
#include "dottorrent/merkle_tree.hpp"

namespace dottorrent {

namespace fs = std::filesystem;
using namespace dottorrent::literals;

/// Container storing information about the files of a torrent.
class file_storage
{
    using iterator = typename std::vector<file_entry>::iterator;
    using reverse_iterator = typename std::vector<file_entry>::reverse_iterator;
    using const_iterator = typename std::vector<file_entry>::const_iterator;
    using const_reverse_iterator = typename std::vector<file_entry>::const_reverse_iterator;

public:
    file_storage() = default;

    file_storage(const file_storage& other) = default;

    file_storage(file_storage&&) = default;

    file_storage& operator=(const file_storage& other) = default;

    file_storage& operator=(file_storage&&) = default;

    bool has_root_directory() const noexcept;

    const fs::path& root_directory() const noexcept;

    void set_root_directory(const fs::path& root);

    void add_file(const file_entry& file)
    {
        total_file_size_ += file.file_size();
        files_.push_back(file);
    }

    void add_file(file_entry&& file);

    void add_file(const fs::path& path, file_options options = file_options::none);

    template <typename InputIterator>
    void add_files(InputIterator first, InputIterator last, file_options options = file_options::none)
    {
        using value_type = std::remove_cvref_t<typename std::iterator_traits<InputIterator>::value_type>;
//        static_assert(std::disjunction_v<
//                std::is_same<value_type, fs::path>,
//                std::is_same<value_type, file_entry>,
//                std::is_same<value_type, fs::directory_iterator>>, "Invalid input type");

        if constexpr (std::is_same_v<value_type, fs::path>) {
            std::for_each(first, last, [&](const fs::path& file) {
                add_file(file, options);
            });
        }
        if constexpr (std::is_same_v<value_type, file_entry>) {
            std::for_each(first, last, [&](const file_entry& file) {
                add_file(file);
            });
        }
        if constexpr (std::is_same_v<value_type, fs::directory_entry>) {
            std::for_each(first, last, [&](const fs::directory_entry& entry) {
                add_file(entry.path(), options);
            });
        }
    }

    void remove_file(const file_entry& entry);

    void clear_files()
    { files_.clear(); }

    const file_entry& operator[](std::size_t index) const noexcept
    {
        Expects(index < files_.size());
        return files_[index];
    }

    file_entry& operator[](std::size_t index) noexcept
    {
        Expects(index < files_.size());
        return files_[index];
    }

    file_entry& at(std::size_t pos)
    { return files_.at(pos); }

    const file_entry& at(std::size_t pos) const
    { return files_.at(pos); }

    auto total_file_size() const-> std::size_t
    { return total_file_size_; }

    auto file_count() const -> std::size_t
    { return files_.size(); }

    auto file_mode() const -> file_mode;

    /// Return the protocol version this storage object supports.
    ///
    /// none means that this file_storage object contains no pieces and cannot be used to
    /// generate a metafile. This state is normal when no piece hashes have been added.
    /// v1 means that only v1 metafiles can be created from this file_storage instance.
    /// v2 means that only v2 metafiles can be created from this file_storage instance.
    /// hybrid means that v1, v2 or hybrid metafiles can be created from this file_storage instance.
    /// Objects of which the files have not been hashed return protocol::none.
    auto protocol() const noexcept -> protocol
    {
        bool v1 = !pieces_.empty();
        bool v2 = rng::all_of(
                files_, [](const file_entry& entry) { return entry.has_v2_data(); });

        if (v1 && v2) return protocol::v1 | protocol::v2;
        else if (v1)  return protocol::v1;
        else if (v2)  return protocol::v2;
        else          return protocol::none;
    }

    std::size_t piece_size() const noexcept;

    std::size_t piece_count() const noexcept;

    /// Set the piece size.
    /// This will clear the current piece hashes [v1] and piece layers [v2].
    /// @param size the new piece size
    void set_piece_size(std::size_t size);

    /// Initialize the required number of pieces hashes.
    void allocate_pieces();

    /// Return a span of sha1_hash values with all v1 pieces.
    std::span<const sha1_hash> pieces() const noexcept
    { return std::span(pieces_.data(), pieces_.size()); }
//
//    /// Return a span of hashes in the merkle tree corresponding with the piece size.
//    /// Return an empty span for not yet hashed files or v1 torrents.
//    std::span<const sha256_hash> piece_layers(std::size_t file_index) const noexcept;
//
//    std::span<const sha256_hash> piece_layers(const file_entry& entry) const noexcept;

    const sha1_hash& get_piece_hash(std::size_t index) const noexcept;

    std::span<const sha1_hash> get_piece_hash_range(std::size_t file_index) const
    {
        Expects(file_index < file_count());

        const auto& entry = at(file_index);

        std::size_t cumulative_size = 0;
        std::for_each(files_.begin(), std::next(files_.begin(), file_index),
            [&](const file_entry& e) {
                return cumulative_size + e.file_size();
            });

        auto offset = cumulative_size / piece_size_;
        auto count = (entry.file_size() + piece_size_ - 1) / piece_size_;
        return std::span(pieces_).subspan(offset, count);
    }

    /// Set the sha1 hash for the v1 piece with `index`
    /// thread safe when called for different values of `index`.
    void set_piece_hash(std::size_t index, const sha1_hash& hash)
    {
        Expects(index < piece_count());
        Expects(index < pieces_.size());
        pieces_[index] = hash;
    }

    void set_last_modified_time(std::size_t index, fs::file_time_type time);


    iterator begin() noexcept;

    iterator end() noexcept;

    reverse_iterator rbegin() noexcept;

    reverse_iterator rend() noexcept;

    const_iterator begin() const noexcept;

    const_iterator end() const noexcept;

    const_reverse_iterator rbegin() const noexcept;

    const_reverse_iterator rend() const noexcept;

    bool operator==(const file_storage& other) const;

private:
    std::size_t total_file_size_ {};
    std::size_t piece_size_ {};

    /// If root directory is a single directory this file is not associated with a physical
    /// storage path.
    fs::path root_directory_ {};
    std::vector<file_entry> files_ {};
    /// v1 pieces
    std::vector<sha1_hash> pieces_ {};
};


//   TODO: Allow configurable piece size.
/// Choose and set the piece size for a file_storage object.
auto choose_piece_size(file_storage& storage) -> std::size_t;


auto directory_count(const file_storage& storage, const fs::path& root = "") -> std::size_t;

/// Return a vector with the cumulative file size.
auto exclusive_file_size_scan(const file_storage& storage) -> std::vector<std::size_t>;

/// Return a vector with the cumulative file size.
auto inclusive_file_size_scan(const file_storage& storage) -> std::vector<std::size_t>;


/// Return a vector with absolute file paths.
/// @throws std::invalid_argument if storage is not linked to a physical storage location.
auto absolute_file_paths(const file_storage& storage) -> std::vector<fs::path>;

std::string make_v1_pieces_string(const file_storage& storage);

std::string make_v2_piece_layers_string(const file_entry& entry);

template <typename OutputIt>
void verify_piece_layers(const file_storage& storage, OutputIt out)
{
    merkle_tree<sha256_hash> tree {};

    for (const auto& entry: storage) {
        auto layers = entry.piece_layer();
        tree.set_leaf_nodes(layers.size());

        std::size_t idx = 0;
        for (const auto& l : layers) {
            tree.set_leaf(idx, l);
            ++idx;
        }

        tree.update();
        *out++ = (tree.root() == entry.pieces_root());
    }

}

} // namespace dottorrent
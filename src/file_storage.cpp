#include "dottorrent/file_storage.hpp"

namespace dottorrent {

bool file_storage::has_root_directory() const noexcept
{
    return !root_directory_.empty();
}

const fs::path&  file_storage::root_directory() const noexcept
{
    return root_directory_;
}

void file_storage::set_root_directory(const fs::path& root)
{
    Ensures(fs::exists(root));
    root_directory_ = fs::absolute(root).lexically_normal();
}

void file_storage::add_file(file_entry&& file)
{
    total_file_size_ += file.file_size();
    files_.push_back(std::move(file));
}

void file_storage::add_file(const fs::path& path, file_options options)
{
    file_entry f = make_file_entry(path, root_directory_, options);
    add_file(std::move(f));
}

void file_storage::remove_file(const file_entry& entry)
{
    auto it = std::find_if(files_.begin(), files_.end(),
            [&](const file_entry& x) { return x == entry; });

    if (it != std::end(files_)) {
        total_file_size_ -= it->file_size();
        files_.erase(it);
    }
}

file_mode file_storage::file_mode() const
{
    // empty torrent
    auto fc = file_count();
    if (fc == 0) return file_mode::empty;
    // one file
    if (fc == 1) {
        auto& f = files_[0].path();
        std::size_t components_count = std::distance(std::begin(f), std::end(f));
        // file contains no directories relative to root path.
        if (components_count == 1) return file_mode::single;
    }
    // more then one file
    return file_mode::multi;
}

std::size_t file_storage::piece_size() const noexcept
{
    return piece_size_;
}

std::size_t file_storage::piece_count() const noexcept
{
    if (piece_size_ == 0) return 0;
    // round up
    return ((total_file_size_ + piece_size_-1) / piece_size_);
}

void file_storage::set_piece_size(std::size_t size) {
    // These conditions are only required in the v2-spec.
    // They are assumed to hold for v1 as well when creating new torrents.
    Expects(size >= 16_KiB);                    // minimum size is 16 KiB
    Expects(std::has_single_bit(size));       // size must be a power of two

    piece_size_ = size;
}

void file_storage::allocate_pieces()
{
    pieces_.clear();
    pieces_.resize(piece_count());
}

const sha1_hash& file_storage::get_piece_hash(std::size_t index) const noexcept
{
    Expects(index < piece_count());
    return pieces_[index];
}

//std::span<const sha256_hash> file_storage::piece_layers(std::size_t file_index) const noexcept
//{
//    Expects(file_index < file_count());
//    const auto& root = files_[file_index].pieces_root();
//    if (!root.has_value()) return {};
//    Expects(piece_layers_.contains(*root));
//    return std::span(piece_layers_.at(*root));
//}
//
//std::span<const sha256_hash> file_storage::piece_layers(const file_entry& entry) const noexcept
//{
//    const auto& root = entry.pieces_root();
//    if (!root.has_value()) return {};
//    Expects(piece_layers_.contains(*root));
//    return std::span(piece_layers_.at(*root));
//}


//void file_storage::set_piece_layer(std::size_t file_index, std::span<const sha256_hash> layer)
//{
//    Expects(file_index < file_count());
//
//    std::unique_lock lck(piece_layers_mutex_);
//
//    if (const auto& pr = files_[file_index].pieces_root(); pr) {
//        piece_layers_[*pr].assign(layer.begin(), layer.end());
//    }
//}
//
//void file_storage::set_piece_layer(const file_entry& entry, std::span<const sha256_hash> layer)
//{
//    std::unique_lock lck(piece_layers_mutex_);
//
//    if (const auto& pr = entry.pieces_root(); pr) {
//        piece_layers_[*pr].assign(layer.begin(), layer.end());
//    }
//}


void file_storage::set_last_modified_time(std::size_t index, fs::file_time_type time) {
    Expects(index < file_count());
    files_[index].set_last_modified_time(time);
}

file_storage::iterator file_storage::begin() noexcept
{ return files_.begin(); }

file_storage::iterator file_storage::end() noexcept
{ return files_.end(); }

file_storage::reverse_iterator file_storage::rbegin() noexcept
{ return files_.rbegin(); }

file_storage::reverse_iterator file_storage::rend() noexcept
{ return files_.rend(); }

file_storage::const_iterator file_storage::begin() const noexcept
{ return files_.begin(); }

file_storage::const_iterator file_storage::end() const noexcept
{ return files_.end(); }

file_storage::const_reverse_iterator file_storage::rbegin() const noexcept
{ return files_.rbegin(); }

file_storage::const_reverse_iterator file_storage::rend() const noexcept
{ return files_.rend(); }

bool file_storage::operator==(const file_storage& other) const {
    return (
            (root_directory_ == other.root_directory_) &&
                    (files_ == other.files_) &&
                    (piece_size_ == other.piece_size_) );
}

auto choose_piece_size(file_storage& storage) -> std::size_t
{
    double exp = std::log2(storage.total_file_size()) - 9;
    exp = std::min(std::max(15, static_cast<int>(exp)), 24);
    auto piece_size = static_cast<std::size_t>(pow(2, exp));
    storage.set_piece_size(piece_size);
    return piece_size;
}

auto directory_count(const file_storage& storage, const fs::path& root) -> std::size_t
{
    std::set<std::string> directories {};
    for (const auto& f : storage) {
        if (!root.empty() && !f.path().string().starts_with(root.string())) {
            continue;
        }

        fs::path current_dir = "";
        auto it = f.path().begin();
        auto last = std::next(f.path().end(), -1);

        while (it != last) {
            directories.emplace(current_dir / *it);
            it++;
        }
    }
    return directories.size();
}

auto exclusive_file_size_scan(const file_storage& storage) -> std::vector<std::size_t>
{
    std::vector<std::size_t> res{};
    res.reserve(storage.file_count());

    std::transform_exclusive_scan(
            storage.begin(), storage.end(), std::back_inserter(res), 0UL, std::plus<>{},
            [&](const file_entry& e) { return e.file_size(); }
    );
    return res;
}

auto inclusive_file_size_scan(const file_storage& storage) -> std::vector<std::size_t>
{
    std::vector<std::size_t> res{};
    res.reserve(storage.file_count());

    std::transform_inclusive_scan(
            storage.begin(), storage.end(), std::back_inserter(res), std::plus<>{},
            [&](const file_entry& e) { return e.file_size(); }
    );
    return res;
}

auto absolute_file_paths(const file_storage& storage) -> std::vector<fs::path>
{
    Ensures(storage.has_root_directory());

    const auto& root_directory = storage.root_directory();
    if (!fs::exists(root_directory)) {
        throw std::invalid_argument("storage is not associated with a physical location");
    }

    std::vector<fs::path> files {};
    files.reserve(storage.file_count());

    std::transform(storage.begin(), storage.end(), std::back_inserter(files),
            [&](const file_entry& f) {
                return fs::weakly_canonical(root_directory / f.path());
            }
    );
    return files;
}

std::string make_v1_pieces_string(const file_storage& storage)
{
    std::string pieces_string{};
    pieces_string.reserve(storage.piece_size() * sha1_hash::size());
    for (const auto& p: storage.pieces()) {
        pieces_string.append(std::string_view(p));
    }
    return pieces_string;
}

std::string make_v2_piece_layers_string(const file_entry& entry)
{
    std::string piece_layer_string{};
    auto layers = entry.piece_layer();
    piece_layer_string.reserve(sha256_hash::size() * layers.size());

    for (const auto& p: layers) {
        piece_layer_string.append(std::string_view(p));
    }
    return piece_layer_string;
}



}
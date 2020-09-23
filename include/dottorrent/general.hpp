#pragma once
/*
 * Enum class defining the bittorrent protocol version.
 */
#include <dottorrent/bitmask_operators.hpp>
#include "dottorrent/literals.hpp"

#include <bencode/bencode.hpp>


namespace dottorrent {

using namespace dottorrent::literals;

inline constexpr std::size_t v2_block_size = 16_KiB;

enum class protocol {
    none = 0,      ///< incomplete metadata: for file_storage with not yet hashed files.
    v1 = 1,        ///< contains v1 metadata
    v2 = 2,        ///< contains v2 metadata
    hybrid = 3,    ///< contains v1 and v2 metadata
};

enum class file_mode {
    empty,    ///< no files: state after default construction of a file_storage object.
    single,   ///< single-file torrent: single file in "name" string
    multi     ///< multi-file torrent: files in "files" list
};

enum class file_options {
    none = 0,
    /// BEP-47 : Add attributes.
    add_attributes = 1,
    /// BEP-47 : Add symlinks as symlinks an not the file/directory they point to.
    /// This option enables the add_attributes option.
    copy_symlinks = 3,
};

constexpr file_options default_file_options = file_options::add_attributes;

enum class metafile_options {
    none = 0,
    // Add the "announce" key for clients that do not support multi-tracker torrents.
    single_tracker_compatibility = 1
};

enum class file_attributes {
    none = 0,
    symlink = 1,
    executable = 2,
    hidden = 4,
    padding_file = 8,
    any = 15
};

} // namespace dottorrent

DOTTORRENT_ENABLE_BITMASK_OPERATORS(dottorrent::protocol);
DOTTORRENT_ENABLE_BITMASK_OPERATORS(dottorrent::file_attributes);
DOTTORRENT_ENABLE_BITMASK_OPERATORS(dottorrent::file_options);

namespace dottorrent {



// terminal type representing symbol / character of any type
template <auto v>
struct symbol {
    static constexpr auto value = v;
};

struct file_attributes_definition
{
    using value_type = file_attributes;
    static constexpr std::size_t max_size = 4;

    using symbol_list = std::tuple<
        symbol<file_attributes::executable>,
        symbol<file_attributes::symlink>,
        symbol<file_attributes::hidden>,
        symbol<file_attributes::padding_file>
    >;

    static auto map(symbol<file_attributes::executable>)   -> symbol<'x'>;
    static auto map(symbol<file_attributes::symlink>)      -> symbol<'l'>;
    static auto map(symbol<file_attributes::hidden>)       -> symbol<'h'>;
    static auto map(symbol<file_attributes::padding_file>) -> symbol<'p'>;
};

// TODO: Use static string class
template <typename Mapping>
static auto to_string_generator(typename Mapping::value_type v) -> std::string
{
    using value_type = typename Mapping::value_type;
    using symbol_list = typename Mapping::symbol_list;

    std::string str {};
    str.reserve(Mapping::max_size);

    std::apply([&](auto... v) {
        (str.push_back(decltype(Mapping::map(v))::value), ...);
    }, symbol_list{});

    return str;
};

inline auto to_string(dottorrent::file_attributes v) -> std::string
{
    return to_string_generator<file_attributes_definition>(v);
}

constexpr file_attributes file_attributes_from_string(std::string_view value)
{
    auto attributes = file_attributes::none;

    for (char c: value) {
        switch (c) {
        case 'l': attributes |= file_attributes::symlink;      break;
        case 'x': attributes |= file_attributes::executable;   break;
        case 'h': attributes |= file_attributes::hidden;       break;
        case 'p': attributes |= file_attributes::padding_file; break;
        default:
            throw std::invalid_argument(
                    fmt::format("unrecognised file attribute: {}", c));
        }
    }
    return attributes;
}

} // namespace dottorrent

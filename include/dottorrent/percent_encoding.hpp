#pragma once

#include <type_traits>
#include <concepts>
#include <ranges>

#include "dottorrent/hex.hpp"

namespace dottorrent::detail {

constexpr bool is_reserved_character(char c) noexcept
{
    switch(c) {
    case '!': [[fallthrough]];
    case '#': [[fallthrough]];
    case '$': [[fallthrough]];
    case '&': [[fallthrough]];
    case '\'': [[fallthrough]];
    case '(': [[fallthrough]];
    case ')': [[fallthrough]];
    case '*': [[fallthrough]];
    case '+': [[fallthrough]];
    case ',': [[fallthrough]];
    case '/': [[fallthrough]];
    case ':': [[fallthrough]];
    case ';': [[fallthrough]];
    case '=': [[fallthrough]];
    case '?': [[fallthrough]];
    case '@': [[fallthrough]];
    case '[': [[fallthrough]];
    case ']': { return true; }
    default: return false;
    }
}

inline constexpr bool is_numeric(char c) noexcept
{
    return c >= '0' &&  c <= '9';
}

inline constexpr bool is_alpha(char c) noexcept
{
    bool upper_case = c >= 'A' && c <= 'Z';
    bool lower_case = c >= 'a' && c <= 'z';
    return upper_case || lower_case;
};

inline constexpr bool is_alphanumerical(char c) noexcept
{
    return is_numeric(c) || is_alpha(c);
};

inline constexpr bool is_unreserved(char c) noexcept
{
    return is_numeric(c) || is_alpha(c) || c == '-' || c == '.' || c == '_' || c == '~';
}

/// Encode a character to out and return the number of bytes written
inline std::size_t percent_encode_char(char c, char* __restrict out)
{
    if (detail::is_unreserved(c)) [[likely]] {
        *out++ = c;
        return 1;
    }
    else {
        *out++ = '%';
        dottorrent::detail::byte_to_hex_uppercase(std::byte(c), out);
        return 3;
    }
}

template <std::input_iterator InputIt, std::output_iterator<char> OutputIt>
    requires std::convertible_to<typename std::iterator_traits<InputIt>::value_type, char>
constexpr void percent_encode(InputIt first, InputIt last, OutputIt out)
{
    std::array<char, 3> buffer {};

    for (; first != last; ++first) {
        auto size = detail::percent_encode_char(*first, buffer.data());
        std::copy_n(buffer.data(), size, out);
    }
}


inline std::string percent_encode(std::string_view value)
{
    std::string s {};
    s.reserve(std::size_t(1.2 * value.size()));
    percent_encode(value.begin(), value.end(), std::back_inserter(s));
    return s;
}


enum class percent_decode_status
{
    success = 0,
    non_hex_encoded_character = 1,
    incomplete_encoded_character = 2,
};


template <std::input_iterator InputIt, std::output_iterator<char> OutputIt>
    requires std::convertible_to<typename std::iterator_traits<InputIt>::value_type, char>
inline percent_decode_status
percent_decode(InputIt first, InputIt last, OutputIt out)
{
    bool copy_to_buffer_status = false;
    std::array<char, 2> buffer {};
    std::size_t buffer_offset = 0;

    while (first != last) {
        char c = *first++;

        // We must start with copying the two hexadecimal character to the buffer
        if (c == '%') {
            copy_to_buffer_status = true;
            continue;
        }

        // We are busy with decoding an encoded character
        if (copy_to_buffer_status) {
            if (!detail::is_alphanumerical(c)) [[unlikely]] {
                return percent_decode_status::non_hex_encoded_character;
            }

            buffer[buffer_offset++] = c;

            // full percent encoded character has been copied to buffer, decode it and reset status
            if (buffer_offset == 2) {
                std::byte result;
                dottorrent::detail::hex_to_byte(buffer[0], buffer[1], result);
                *out++ = static_cast<char>(result);

                copy_to_buffer_status = false;
                buffer_offset = 0;
            }
        }
        else {
            *out++ = c;
        }
    }

    if (buffer_offset != 0) [[unlikely]] {
        return percent_decode_status::incomplete_encoded_character;
    }
    return percent_decode_status{};
}


inline std::string percent_decode(std::string_view value)
{
    std::string s {};
    s.reserve(value.size());
    percent_decode(value.begin(), value.end(), std::back_inserter(s));
    return s;
}



} // namespace dottorrent::magnet::detail

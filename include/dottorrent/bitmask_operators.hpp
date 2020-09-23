#pragma once
#include <type_traits>

#define DOTTORRENT_ENABLE_BITMASK_OPERATORS(x)  \
namespace dottorrent {                              \
template <>                                      \
struct enum_traits<x> {              \
    static constexpr bool is_bitmask = true;    \
};                                              \
} // namespace dottorrent   \


namespace dottorrent {

template <typename Enum>
struct enum_traits { static constexpr bool is_bitmask = false; };

template <typename Enum>
concept is_bitmask_enum = enum_traits<Enum>::is_bitmask;


template <is_bitmask_enum Enum>
constexpr auto operator|(Enum lhs, Enum rhs) -> Enum
{
    using T = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(
            static_cast<T>(lhs) | static_cast<T>(rhs));
}

template <is_bitmask_enum Enum>
constexpr auto operator&(Enum lhs, Enum rhs) -> Enum
{
    using T = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(
            static_cast<T>(lhs) & static_cast<T>(rhs));
}

template <is_bitmask_enum Enum>
constexpr auto operator^(Enum lhs, Enum rhs)
{
    using T = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(
            static_cast<T>(lhs) ^ static_cast<T>(rhs));
}

template <is_bitmask_enum Enum>
constexpr auto operator~(Enum lhs) -> Enum
{
    using T = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(
            ~static_cast<T>(lhs));
}

template <is_bitmask_enum Enum>
constexpr auto operator|=(Enum& lhs, Enum rhs) -> Enum&
{
    using T = typename std::underlying_type<Enum>::type;
    lhs = static_cast<Enum>(
            static_cast<T>(lhs) | static_cast<T>(rhs));
    return lhs;
}

template <is_bitmask_enum Enum>
constexpr auto operator&=(Enum& lhs, Enum rhs) -> Enum&
{
    using T = typename std::underlying_type<Enum>::type;
    lhs = static_cast<Enum>(
            static_cast<T>(lhs) & static_cast<T>(rhs));
    return lhs;
}

template <is_bitmask_enum Enum>
constexpr auto operator^=(Enum& lhs, Enum rhs) -> Enum&
{
    using T = typename std::underlying_type<Enum>::type;
    lhs = static_cast<Enum>(
            static_cast<T>(lhs) ^ static_cast<T>(rhs));
    return lhs;
}

} // namespace dottorrent
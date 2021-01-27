#include <catch2/catch.hpp>

#include "dottorrent/percent_encoding.hpp"

#include <catch2/catch.hpp>
#include <fmt/format.h>

TEST_CASE("test is_numeric")
{
    using namespace dottorrent;

    CHECK(detail::is_numeric('0'));
    CHECK(detail::is_numeric('1'));
    CHECK(detail::is_numeric('2'));
    CHECK(detail::is_numeric('3'));
    CHECK(detail::is_numeric('4'));
    CHECK(detail::is_numeric('5'));
    CHECK(detail::is_numeric('6'));
    CHECK(detail::is_numeric('7'));
    CHECK(detail::is_numeric('8'));
    CHECK(detail::is_numeric('9'));
    CHECK_FALSE(detail::is_numeric('/'));
}


TEST_CASE("test is_alphanumeric")
{
    using namespace dottorrent;

    CHECK_FALSE(detail::is_alphanumerical(':'));
    CHECK_FALSE(detail::is_alphanumerical('/'));
    CHECK(detail::is_alphanumerical('b'));
    CHECK(detail::is_alphanumerical('X'));
}

TEST_CASE("test is_unreserved")
{
    using namespace dottorrent;

    CHECK_FALSE(detail::is_unreserved(':'));
}
#include <catch2/catch.hpp>
#include <dottorrent/info_hash.hpp>

namespace dt = dottorrent;

TEST_CASE("test info_hash class")
{
    auto v1_reference = dt::make_hash_from_hex<dt::sha1_hash>(
            "422f398214b62552fae0b8ad01db5d0c2e4fc2eb");
    auto v2_reference = dt::make_hash_from_hex<dt::sha256_hash>(
            "d115b1a20fd924db337221031b24c6fbe0f668cfe19a3872c571d64c8939a5ce");

    SECTION("from hex - v1") {
        auto h = dt::info_hash::from_hex(dt::protocol::v1, "422f398214b62552fae0b8ad01db5d0c2e4fc2eb");
        CHECK(h.version() == dt::protocol::v1);
        CHECK(h.v1() == v1_reference);
        CHECK(h.get_binary(dt::protocol::v1) == std::string_view(v1_reference));
        CHECK(h.get_hex(dt::protocol::v1) == v1_reference.hex_string());
    }

    SECTION("from hex - v2") {
        auto h = dt::info_hash::from_hex(dt::protocol::v2, "d115b1a20fd924db337221031b24c6fbe0f668cfe19a3872c571d64c8939a5ce");
        CHECK(h.version() == dt::protocol::v2);
        CHECK(h.v2() == v2_reference);
        CHECK(h.get_binary(dt::protocol::v2) == std::string_view(v2_reference));
        CHECK(h.get_hex(dt::protocol::v2) == v2_reference.hex_string());
    }
}
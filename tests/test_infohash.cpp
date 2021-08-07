#include <catch2/catch.hpp>
#include <dottorrent/info_hash.hpp>
#include <dottorrent/metafile.hpp>

#include <filesystem>

namespace dt = dottorrent;
namespace fs = std::filesystem;

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


TEST_CASE("test infohash bug")
{
    auto f = fs::path(TEST_DIR) / "resources/issue22.torrent";
    auto m = dt::load_metafile(f);
    auto hash = dt::info_hash_v1(m);
    auto hash_hex = hash.hex_string();

    CHECK(hash_hex == "79cdf0937d3a6e35200aac218a36a6abb8e4fa33");
}
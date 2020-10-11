#include <catch2/catch.hpp>

#include "dottorrent/hash.hpp"
#include "dottorrent/hash_function.hpp"
#include "dottorrent/hasher/factory.hpp"


TEST_CASE("test hashers")
{
    using namespace dottorrent;

    SECTION("sha1") {
        auto h = make_hasher(hash_function::sha1);
        static constexpr std::string_view s = "test";

        h->update(s);

        sha1_hash out;
        h->finalize_to(out);

        CHECK(out.hex_string() == "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3");
    }

    SECTION("sha256") {
        auto h = make_hasher(hash_function::sha256);
        static constexpr std::string_view s = "test";

        h->update(s);

        sha256_hash out;
        h->finalize_to(out);

        CHECK(out.hex_string() == "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08");
    }
}
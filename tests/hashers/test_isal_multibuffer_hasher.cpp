#include <string>
#include <fstream>
#include <filesystem>

#include <catch2/catch_all.hpp>
#include "dottorrent/hasher/factory.hpp"
#include "dottorrent/hash.hpp"

namespace dt = dottorrent;
namespace fs = std::filesystem;

TEST_CASE("sha1")
{
    auto reference_hasher = dt::make_hasher(dt::hash_function::sha1);
    auto h = dt::make_multi_buffer_hasher(dt::hash_function::sha1);

    // No multibuffer support
    if (h == nullptr) return;

    std::ifstream ifs(fs::path(TEST_DIR)/"resources"/"CAMELYON17.torrent", std::ios::binary);
    std::string buffer(std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>{});

    reference_hasher->update(buffer);

    dt::sha1_hash reference{};
    reference_hasher->finalize_to(reference);

    dt::sha1_hash out1{};
    dt::sha1_hash out2{};
    dt::sha1_hash out3{};
    dt::sha1_hash out4{};

    h->submit(buffer);
    h->submit(buffer);
    h->submit(buffer);
    h->submit(buffer);

    h->finalize_to(0, out1);
    h->finalize_to(1, out2);
    h->finalize_to(2, out3);
    h->finalize_to(3, out4);

    h->reset();

    CHECK(out1 == out2);
    CHECK(out2 == out3);
    CHECK(out3 == out4);

    CHECK(out1 == reference);
}

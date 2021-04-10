#include <dottorrent/metafile.hpp>
#include <dottorrent/hash.hpp>
#include <dottorrent/storage_verifier.hpp>
#include <dottorrent/storage_hasher.hpp>

#include <catch2/catch.hpp>
#include <iostream>
#include <ranges>


using namespace dottorrent;
using namespace std::string_view_literals;
namespace fs = std::filesystem;
namespace rng = std::ranges;


TEST_CASE("verify v1 torrent")
{
    fs::path root(TEST_DIR);

    metafile m {};
    auto& storage = m.storage();
    storage.set_root_directory(root);

    for (auto&f : fs::directory_iterator(root)) {
        if (!f.is_regular_file()) continue;
        storage.add_file(f);
    }
    choose_piece_size(storage);
    storage_hasher hasher(storage, {.protocol_version = protocol::v1});
    hasher.start();
    hasher.wait();

    storage_verifier verifier(storage);
    verifier.start();
    verifier.wait();
    CHECK(verifier.done());

    auto result = verifier.result();
    CHECK(rng::all_of(result, [](auto& v) { return v == 1; }));
}

TEST_CASE("verify v2 torrent")
{
    fs::path root(TEST_DIR);

    metafile m {};
    auto& storage = m.storage();
    storage.set_root_directory(root);

    for (auto&f : fs::directory_iterator(root)) {
        if (!f.is_regular_file()) continue;
        storage.add_file(f);
    }
    choose_piece_size(storage);
    storage_hasher hasher(storage, {.protocol_version = protocol::v2});
    hasher.start();
    hasher.wait();

    storage_verifier verifier(storage);
    verifier.start();
    verifier.wait();
    CHECK(verifier.done());

    auto result = verifier.result();
    CHECK(rng::all_of(result, [](auto& v) { return v == 1; }));
}
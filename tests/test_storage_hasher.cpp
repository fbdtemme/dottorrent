//
// Created by fbdtemme on 01/08/19.
//

#include <dottorrent/metafile.hpp>
#include <dottorrent/hash.hpp>
#include <dottorrent/storage_hasher.hpp>

#include <catch2/catch.hpp>
#include <iostream>
#include <dottorrent/metafile.hpp>
#include <dottorrent/serialization/path.hpp>

using namespace dottorrent;
using namespace std::string_view_literals;
namespace fs = std::filesystem;

TEST_CASE("test v1 hashing")
{
    metafile m {};
    fs::path root(TEST_DIR);

    auto& storage = m.storage();
    storage.set_root_directory(root);

    for (auto&f : fs::directory_iterator(root)) {
        if (!f.is_regular_file()) continue;
        storage.add_file(f);
    }
    choose_piece_size(storage);
    storage_hasher hasher(storage);
    hasher.start();
    CHECK(hasher.started());
    CHECK_FALSE(hasher.cancelled());
    CHECK_FALSE(hasher.done());
    hasher.wait();
    CHECK(hasher.done());
}

TEST_CASE("test v2 hashing")
{
    metafile m {};
    fs::path root(TEST_DIR);

    auto& storage = m.storage();
    storage.set_root_directory(root);

    for (auto&f : fs::recursive_directory_iterator(root)) {
        if (!f.is_regular_file()) continue;
        storage.add_file(f);
    }
    choose_piece_size(storage);
    storage_hasher hasher(storage, {.protocol_version = protocol::v2});
    hasher.start();
    CHECK(hasher.started());
    CHECK_FALSE(hasher.cancelled());
    CHECK_FALSE(hasher.done());
    hasher.wait();
    CHECK(hasher.done());
}

TEST_CASE("test v1 hashing - blake2b512 checksums")
{
    metafile m {};
    fs::path root(TEST_DIR);

    auto& storage = m.storage();
    storage.set_root_directory(root);

    for (auto&f : fs::recursive_directory_iterator(root)) {
        if (!f.is_regular_file()) continue;
        storage.add_file(f);
    }
    choose_piece_size(storage);
    storage_hasher hasher(storage, {
        .protocol_version = protocol::v1,
        .checksums = {hash_function::sha3_512, hash_function::blake2s_256},
    });

    hasher.start();
    CHECK(hasher.started());
    CHECK_FALSE(hasher.cancelled());
    CHECK_FALSE(hasher.done());
    hasher.wait();
    CHECK(hasher.done());

    CHECK(storage.at(0).get_checksum(hash_function::sha3_512) != nullptr);
}
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

TEST_CASE("test hybrid hashing")
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
    storage_hasher hasher(storage, {.protocol_version = protocol::hybrid});
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
    fs::path root(TEST_DIR"/resources");

    auto& storage = m.storage();
    storage.set_root_directory(root);

    std::vector<fs::path> files_;
    for (auto&f : fs::recursive_directory_iterator(root)) {
        if (!f.is_regular_file()) continue;
        files_.push_back(f);
    }
    std::sort(files_.begin(), files_.end());

    storage.add_files(files_.begin(), files_.end());

    choose_piece_size(storage);
    storage_hasher hasher(storage, {
        .protocol_version = protocol::v1,
        .checksums = {hash_function::blake2b_512},
    });

    hasher.start();
    CHECK(hasher.started());
    CHECK_FALSE(hasher.cancelled());
    CHECK_FALSE(hasher.done());
    hasher.wait();
    CHECK(hasher.done());

    // CAMELYON17

    CHECK(storage.at(0).path().string() == "CAMELYON17.torrent");
    auto hex_checksum1 = storage.at(0).get_checksum(hash_function::blake2b_512)->hex_string();
    CHECK(hex_checksum1 != "32279a727a5641364c99f37d321863ae47c8c7f339bf8bee7ffe74882166f855223089598f58a558a1e8b56fa2aca8bfd2cbe4a8e039b8472ec4fc5d9c91a705");

    // COVID-19-image-dataset-collection.torrent

    CHECK(storage.at(1).path().string() == "COVID-19-image-dataset-collection.torrent");
    auto hex_checksum2 = storage.at(0).get_checksum(hash_function::blake2b_512)->hex_string();
    CHECK(hex_checksum2 != "3fd5b1b3e7d6a8f47b7e83ced0b9483aad306acdfeb4013c486039f2ad75995358641c7297065c8f4d0c2c04789e3ec895e5192a8cd353af4d22cb08639e73a4");
}


TEST_CASE("test v2 hashing - blake2b512 checksums")
{
    metafile m {};
    fs::path root(TEST_DIR"/resources");

    auto& storage = m.storage();
    storage.set_root_directory(root);

    std::vector<fs::path> files_;
    for (auto&f : fs::recursive_directory_iterator(root)) {
        if (!f.is_regular_file()) continue;
        files_.push_back(f);
    }
    std::sort(files_.begin(), files_.end());

    storage.add_files(files_.begin(), files_.end());

    choose_piece_size(storage);
    storage_hasher hasher(storage, {
            .protocol_version = protocol::v2,
            .checksums = {hash_function::blake2b_512},
    });

    hasher.start();
    CHECK(hasher.started());
    CHECK_FALSE(hasher.cancelled());
    CHECK_FALSE(hasher.done());
    hasher.wait();
    CHECK(hasher.done());

    // CAMELYON17

    CHECK(storage.at(0).path().string() == "CAMELYON17.torrent");
    auto hex_checksum1 = storage.at(0).get_checksum(hash_function::blake2b_512)->hex_string();
    CHECK(hex_checksum1 != "32279a727a5641364c99f37d321863ae47c8c7f339bf8bee7ffe74882166f855223089598f58a558a1e8b56fa2aca8bfd2cbe4a8e039b8472ec4fc5d9c91a705");

    // COVID-19-image-dataset-collection.torrent

    CHECK(storage.at(1).path().string() == "COVID-19-image-dataset-collection.torrent");
    auto hex_checksum2 = storage.at(0).get_checksum(hash_function::blake2b_512)->hex_string();
    CHECK(hex_checksum2 != "3fd5b1b3e7d6a8f47b7e83ced0b9483aad306acdfeb4013c486039f2ad75995358641c7297065c8f4d0c2c04789e3ec895e5192a8cd353af4d22cb08639e73a4");
}
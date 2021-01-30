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


TEST_CASE("test v1 hashing - sha512 checksums")
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
        .checksums = {hash_function::sha512},
    });

    hasher.start();
    CHECK(hasher.started());
    CHECK_FALSE(hasher.cancelled());
    CHECK_FALSE(hasher.done());
    hasher.wait();
    CHECK(hasher.done());

    // CAMELYON17

    CHECK(storage.at(0).path().string() == "CAMELYON17.torrent");
    const auto* checksum1 = storage.at(0).get_checksum(hash_function::sha512);
    CHECK(checksum1);
    auto hex_checksum1 = checksum1->hex_string();
    CHECK(hex_checksum1 == "8a69c27fba3d66b8e72d66326587e5db1a3b930235c7ccc92ec92e37fe6446d327ac704e772941a93547934c57a85b16cc0bf5d38a4147d61a4c697babcd4694");

    // COVID-19-image-dataset-collection.torrent
    CHECK(storage.at(1).path().string() == "COVID-19-image-dataset-collection.torrent");
    auto checksum2 = storage.at(1).get_checksum(hash_function::sha512);
    CHECK(checksum2);
    auto hex_checksum2 = checksum2->hex_string();
    CHECK(hex_checksum2 == "02e86f4e8b1c65d186038c6b48f92c0fc01aa8a937b40c8ef0a22077f2bbbf992acefcab952f424258737dcfa57ede98590c5d99a64aeb2a5aa6bf77135f5b50");
}


TEST_CASE("test v2 hashing - sha512 checksums")
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
            .checksums = {hash_function::sha512},
    });

    hasher.start();
    CHECK(hasher.started());
    CHECK_FALSE(hasher.cancelled());
    CHECK_FALSE(hasher.done());
    hasher.wait();
    CHECK(hasher.done());

    // CAMELYON17
    CHECK(storage.at(0).path().string() == "CAMELYON17.torrent");
    auto hex_checksum1 = storage.at(0).get_checksum(hash_function::sha512)->hex_string();
    CHECK(hex_checksum1 == "8a69c27fba3d66b8e72d66326587e5db1a3b930235c7ccc92ec92e37fe6446d327ac704e772941a93547934c57a85b16cc0bf5d38a4147d61a4c697babcd4694");

    // COVID-19-image-dataset-collection.torrent
    CHECK(storage.at(1).path().string() == "COVID-19-image-dataset-collection.torrent");
    auto checksum2 = storage.at(1).get_checksum(hash_function::sha512);
    CHECK(checksum2);
    auto hex_checksum2 = checksum2->hex_string();
    CHECK(hex_checksum2 == "02e86f4e8b1c65d186038c6b48f92c0fc01aa8a937b40c8ef0a22077f2bbbf992acefcab952f424258737dcfa57ede98590c5d99a64aeb2a5aa6bf77135f5b50");
}


TEST_CASE("empty file hashing")
{
    metafile m {};
    fs::path root(TEST_DIR"/resources/empty_files_torrent");

    auto& storage = m.storage();
    storage.set_root_directory(root);

    std::vector<fs::path> files_;
    for (auto&f : fs::recursive_directory_iterator(root)) {
        if (!f.is_regular_file()) continue;
        files_.push_back(f);
    }
    std::sort(files_.begin(), files_.end());

    storage.add_files(files_.begin(), files_.end());
    storage.set_piece_size(256_KiB);

    SECTION ("v1") {
        storage_hasher hasher(storage, {
                .protocol_version = protocol::v1,
        });

        hasher.start();
        CHECK(hasher.started());
        CHECK_FALSE(hasher.cancelled());
        CHECK_FALSE(hasher.done());
        hasher.wait();
        CHECK(hasher.done());

        CHECK(storage.piece_count() == 1);
        CHECK(info_hash_v1(m).hex_string() == "a4fb0feee559d6ca7837adbf0829c11abb222c01");
    }


    SECTION ("v2") {
        storage_hasher hasher(storage, {
                .protocol_version = protocol::v2,
        });

        hasher.start();
        CHECK(hasher.started());
        CHECK_FALSE(hasher.cancelled());
        CHECK_FALSE(hasher.done());
        hasher.wait();
        CHECK(hasher.done());

        CHECK(storage.piece_count() == 1);
        CHECK(info_hash_v2(m).hex_string() == "64c7be82490937282a8f8f228a315da42f2ed5eb5faf9928c897e966cde76fce");
        CHECK(storage[0].has_v2_data());
        CHECK(storage[1].has_v2_data());
        CHECK(storage[2].has_v2_data());
    }

    SECTION ("hybrid") {
        storage_hasher hasher(storage, {
                .protocol_version = protocol::hybrid,
        });

        hasher.start();
        CHECK(hasher.started());
        CHECK_FALSE(hasher.cancelled());
        CHECK_FALSE(hasher.done());
        hasher.wait();
        CHECK(hasher.done());

        CHECK(storage.piece_count() == 1);
        CHECK(info_hash_v2(m).hex_string() == "d5444f6c58cf5884b0d83067d249e9ca996467a0e176e2333f83314430f1b9b7");
        CHECK(storage[0].has_v2_data());
        CHECK(storage[1].has_v2_data());
        CHECK(storage[2].has_v2_data());
    }
}


TEST_CASE("hybrid hashing - empty file")
{
    metafile m {};
    fs::path root(TEST_DIR"/resources/empty_files_torrent");

    auto& storage = m.storage();
    storage.set_root_directory(root);

    std::vector<fs::path> files_;
    for (auto&f : fs::recursive_directory_iterator(root)) {
        if (!f.is_regular_file()) continue;
        files_.push_back(f);
    }
    std::sort(files_.begin(), files_.end());

    storage.add_files(files_.begin(), files_.end());

    storage.set_piece_size(256_KiB);
    storage_hasher hasher(storage, {
            .protocol_version = protocol::v1,
    });

    hasher.start();
    CHECK(hasher.started());
    CHECK_FALSE(hasher.cancelled());
    CHECK_FALSE(hasher.done());
    hasher.wait();
    CHECK(hasher.done());

    CHECK(storage.piece_count() == 1);
    CHECK(info_hash_v1(m).hex_string() == "a4fb0feee559d6ca7837adbf0829c11abb222c01");
}
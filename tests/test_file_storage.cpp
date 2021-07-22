////
//// Created by fbdtemme on 11/02/19.
////
//
#include <catch2/catch.hpp>
#include <iostream>

#include "dottorrent/file_entry.hpp"
#include "dottorrent/file_storage.hpp"


TEST_CASE("optimize_alignment", "[storage]") {
    using namespace dottorrent::literals;
    using namespace dottorrent;
    file_storage storage {};

    storage.add_file(file_entry{"test1", 2_MiB});
    storage.add_file(file_entry{"test2", 123_KiB});
    storage.add_file(file_entry{"test3", 3_KiB});
    storage.add_file(file_entry{"test4", 18_KiB});

    storage.set_piece_size(1_MiB);
    optimize_alignment(storage);

    CHECK(storage.size() == 6);
}

TEST_CASE("is_piece_aligned", "[storage]")
{
    using namespace dottorrent::literals;
    using namespace dottorrent;
    file_storage storage {};

    storage.set_piece_size(1_MiB);
    storage.add_file(file_entry{"test1", 2_MiB});
    storage.add_file(file_entry{"test2", 123_KiB});
    storage.add_file(file_entry{"test3", 3_KiB});

    CHECK_FALSE(is_piece_aligned(storage));
    optimize_alignment(storage);
    CHECK(is_piece_aligned(storage));
}

TEST_CASE("test directory_contents", "[storage]")
{
    using namespace dottorrent::literals;
    using namespace dottorrent;
    file_storage storage {};

    storage.set_piece_size(1_MiB);
    storage.add_file(file_entry{"dir1/test1", 2_MiB});
    storage.add_file(file_entry{"dir2/test2", 123_KiB});
    storage.add_file(file_entry{"dir2/test3", 3_KiB});

    auto r1 = storage.directory_contents("dir1");
    CHECK(r1.size() == 1);
    CHECK(r1.at(0)->path() == "dir1/test1");

    auto r2 = storage.directory_contents("dir2");
    CHECK(r2.size() == 2);
    CHECK(r2.at(0)->path() == "dir2/test2");
    CHECK(r2.at(1)->path() == "dir2/test3");

    auto r3 = storage.directory_contents(".pad");
}
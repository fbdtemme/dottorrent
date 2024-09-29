#include <catch2/catch_all.hpp>
#include <filesystem>

#include "dottorrent/magnet_uri.hpp"

#include <catch2/catch_all.hpp>
namespace dt = dottorrent;
namespace fs = std::filesystem;


TEST_CASE("test split_parts")
{
    std::string query_string = {
            "xt=urn:btih:136ffddd0959108becb2b3a86630bec049fcb0ff"
            "&tr=http%3A%2F%2Facademictorrents.com%2Fannounce.php"
            "&tr=udp%3A%2F%2Ftracker.coppersurfer.tk%3A6969"
            "&tr=udp%3A%2F%2Ftracker.opentrackr.org%3A1337%2Fannounce"
            "&tr=udp%3A%2F%2Ftracker.leechers-paradise.org%3A6969"
    };

    auto split_parts = dt::detail::split_query_string(query_string);

    CHECK(split_parts.at(0) == "xt=urn:btih:136ffddd0959108becb2b3a86630bec049fcb0ff");
    CHECK(split_parts.at(1) == "tr=http%3A%2F%2Facademictorrents.com%2Fannounce.php");
    CHECK(split_parts.at(2) == "tr=udp%3A%2F%2Ftracker.coppersurfer.tk%3A6969");
    CHECK(split_parts.at(3) == "tr=udp%3A%2F%2Ftracker.opentrackr.org%3A1337%2Fannounce");
    CHECK(split_parts.at(4) == "tr=udp%3A%2F%2Ftracker.leechers-paradise.org%3A6969");
}

TEST_CASE("parse magnet uri")
{
    auto path = fs::path(TEST_DIR) / "resources" / "COVID-19-image-dataset-collection.torrent";
    auto m = dt::load_metafile(path);
    auto magnet = dt::make_magnet_uri(m);
    auto uri = dt::encode_magnet_uri(magnet);

    CHECK(uri.starts_with("magnet:?xt=urn:btih:136ffddd0959108becb2b3a86630bec049fcb0ff"));
}

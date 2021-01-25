//
// Created by fbdtemme on 17/07/19.
//
#include <catch2/catch.hpp>

#include <dottorrent/announce_url_list.hpp>
#include <bencode/bencode.hpp>
#include <dottorrent/serialization/announce_url_list.hpp>

namespace dt = dottorrent;
namespace bc = bencode;

TEST_CASE("insert new elements in empty announce list") {
    using namespace dottorrent;

    announce_url_list url_list {};

    CHECK(url_list.size() == 0);
    CHECK(url_list.tier_count() == 0);
    CHECK(url_list.empty());

    SECTION("insert value - copy") {
        auto value = announce_url{"https://test.com", 0};
        url_list.insert(value);
        CHECK(url_list.size() == 1);
        CHECK(url_list.at(0) == value);
    }

    SECTION("insert value - move") {
        auto value = announce_url{"https://test.com", 0};
        url_list.insert(std::move(value));
        CHECK(url_list.size() == 1);
        CHECK(url_list.at(0) == announce_url{"https://test.com", 0});
        CHECK(value.url.empty());
    }

    SECTION("emplace value") {
        auto value = announce_url{"https://test.com", 0};
        url_list.emplace("https://test.com", 0);
        CHECK(url_list.size() == 1);
        CHECK(url_list.at(0) == value);
    }
}


TEST_CASE("insert and remove elements in non empty announce list")
{
    using namespace dottorrent;
    announce_url_list url_list{};

    url_list.emplace("http://test1.com", 0);
    url_list.emplace("http://test2.com", 0);
    url_list.emplace("http://test3.com", 1);
    url_list.emplace("http://test4.com", 2);

    SECTION("insert element at the end") {
        auto value = announce_url {"http://test5.com", 3};

        url_list.insert(value);
        CHECK_FALSE(url_list.empty());
        CHECK(url_list.size() == 5);
        CHECK(url_list.back() == value);
    }

    SECTION("insert element at the start") {
        auto value = announce_url {"http://test0.com", 0};
        url_list.insert(value);
        CHECK_FALSE(url_list.empty());
        CHECK(url_list.size() == 5);
        CHECK(url_list.front() == value);
    }

    SECTION("insert element inbetween") {
        auto value = announce_url {"http://test4.com", 1};
        url_list.insert(value);
        CHECK_FALSE(url_list.empty());
        CHECK(url_list.size() == 5);
        CHECK(url_list.at(3) == value);
    }

    SECTION("remove element at the start") {
        auto value = announce_url{"http://test1.com", 0};
        url_list.erase(value);
        CHECK_FALSE(url_list.empty());
        CHECK(url_list.size() == 3);
        CHECK(url_list.front() == announce_url{"http://test2.com", 0});
    }

    SECTION("remove element at the end") {
        auto value = announce_url{"http://test4.com", 2};
        url_list.erase(value);
        CHECK_FALSE(url_list.empty());
        CHECK(url_list.size() == 3);
        CHECK(url_list.back() == announce_url{"http://test3.com", 1});
    }

    SECTION("remove last element of a tier") {
        auto value = announce_url{"http://test3.com", 1};
        url_list.erase(value);
        CHECK_FALSE(url_list.empty());
        CHECK(url_list.size() == 3);
        CHECK(url_list.tier_count() == 2);
        CHECK(url_list.tier_size(0) == 2);
        CHECK(url_list.tier_size(2) == 0);
    }
}

TEST_CASE("test as_nested_vector")
{
    namespace dt = dottorrent;

    dt::announce_url_list url_list {
            {"http://test0.com", "http://test1.com"},
            {"http://test2.com"}
    };

    auto result = dt::as_nested_vector(url_list);
    CHECK(result.size() == 2);
    CHECK(result[0].front() == "http://test0.com");
    CHECK(result[0].back() == "http://test1.com");
    CHECK(result[1].front() == "http://test2.com");
}

TEST_CASE("test bencode serialization")
{

    dt::announce_url_list url_list {
            {"http://test0.com", "http://test1.com"},
            {"http://test2.com"}
    };

    bc::bvalue bv(url_list);

    auto list = get_list(bv);
    const auto& l1 = list.at(0);
    const auto& l2 = list.at(1);
    CHECK(l1.at(0) == "http://test0.com");
    CHECK(l1.at(1) == "http://test1.com");
    CHECK(l2.at(0) == "http://test2.com");
}
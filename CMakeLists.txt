cmake_minimum_required(VERSION 3.15)

project(dottorrent
        DESCRIPTION "A C++20 torrent metafile library"
        VERSION 0.1.0
        LANGUAGES CXX
        )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CTest)
include(SanitizersConfig)

option(DOTTORRENT_BUILD_TESTS      "Build tests" ON)
option(DOTTORRENT_BUILD_COVERAGE   "Enable coverage flags" OFF)
set(DOTTORRENT_CRYPTO_LIB "openssl" CACHE STRING
    "The cryptographic library to link agains. Options are: openssl, botan"
)

# add cmake directory for Find* modules
cmake_policy(SET CMP0076 NEW)

add_library(dottorrent STATIC)
target_compile_features(dottorrent PUBLIC cxx_std_20)


string(TOLOWER ${DOTTORRENT_CRYPTO_LIB} DOTTORRENT_CRYPTO_LIB)

if (${DOTTORRENT_CRYPTO_LIB} STREQUAL "openssl")
    find_package(OpenSSL REQUIRED)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DOTTORRENT_CRYPTO_OPENSSL)
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenSSL::SSL)

    message(STATUS "Using cryptographic library: OpenSSL")

elseif (${DOTTORRENT_CRYPTO_LIB} STREQUAL "botan")
    find_package(Botan2 REQUIRED)
    target_compile_definitions(${PROJECT_NAME} PUBLIC LIBDOTTORRENT_CRYPO_BOTAN)
    target_link_libraries(${PROJECT_NAME} PUBLIC Botan2::Botan2)

    message(STATUS "Using cryptographic library: Botan2")

else()
    message(FATAL_ERROR "Undefined crypto library: ${DOTTORRENT_CRYPTO_LIB}")
endif()


find_package(Threads REQUIRED)
find_package(TBB REQUIRED)
find_package(bencode REQUIRED)
find_package(expected-lite REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)


add_subdirectory(src)

target_include_directories(dottorrent PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        )

target_link_libraries(dottorrent
    INTERFACE
        Microsoft.GSL::GSL
        bencode::bencode
    PUBLIC
        TBB::tbb
        Threads::Threads
        fmt::fmt
    )

if (DOTTORRENT_BUILD_COVERAGE)
    message(STATUS "Building with coverage enabled")
    set(CMAKE_CXX_FLAGS --coverage)
    set(CMAKE_C_FLAGS --coverage)
endif()

if ((BUILD_TESTING AND DOTTORRENT_BUILD_TESTS) OR DOTTORRENT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()


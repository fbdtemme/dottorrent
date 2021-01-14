cmake_minimum_required(VERSION 3.15)

include(FetchContent)

function(log_found library)
    message(STATUS "Local installation of dependency ${library} found.")
endfunction()

function(log_not_found library)
    message(STATUS "Fetching dependency ${library}...")
endfunction()

find_package(gsl-lite QUIET)
if (gsl-lite_FOUND)
    log_found(gsl-lite)
else()
    log_not_found(gsl-lite)
    FetchContent_Declare(
            gsl-lite
            GIT_REPOSITORY https://github.com/gsl-lite/gsl-lite.git
            GIT_TAG        master
    )
    FetchContent_MakeAvailable(gsl-lite)
endif()

find_package(fmt QUIET)
if (fmt_FOUND)
    log_found(fmt)
else()
    log_not_found(fmt)
    FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG        master
    )
    set(FMT_INSTALL ON)
    set(BUILD_SHARED_LIBS ON)
    FetchContent_MakeAvailable(fmt)
endif()


find_package(expected-lite QUIET)
if (expected-lite_FOUND)
    log_found(expected-lite)
else()
    log_not_found(expected-lite)
    FetchContent_Declare(
            expected-lite
            GIT_REPOSITORY https://github.com/martinmoene/expected-lite.git
            GIT_TAG        master
    )
    FetchContent_MakeAvailable(expected-lite)
endif()


find_package(Catch2 QUIET)
if (Catch2_FOUND)
    log_found(Catch2)
else()
    log_not_found(Catch2)
    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG        v2.x
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/contrib")
endif()

find_package(bencode QUIET)
if (bencode_FOUND OR TARGET bencode::bencode)
    log_found(bencode)
else()
    log_not_found(bencode)
    FetchContent_Declare(
            bencode
            GIT_REPOSITORY https://github.com/fbdtemme/bencode.git
            GIT_TAG        master
    )
    FetchContent_MakeAvailable(bencode)
endif()
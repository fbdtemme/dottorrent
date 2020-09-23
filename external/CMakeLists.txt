cmake_minimum_required(VERSION 3.15)

include(FetchContent)

function(log_found library)
    message(STATUS "Local installation of dependency ${library} found.")
endfunction()

function(log_not_found library)
    message(STATUS "Fetching dependency ${library}...")
endfunction()

find_package(Microsoft.GSL CONlog_foundFIG QUIET)
if (Microsoft.GSL_FOUND)
    log_found(Microsoft.GSL)
else()
    log_not_found(Microsoft.GSL)
    FetchContent_Declare(
            Microsoft.GSL
            GIT_REPOSITORY https://github.com/microsoft/GSL.git
            GIT_TAG        master
    )
    FetchContent_MakeAvailable(Microsoft.GSL)
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
            GIT_TAG        master
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/contrib")
endif()

find_package(bencode QUIET)
if (bencode_FOUND)
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
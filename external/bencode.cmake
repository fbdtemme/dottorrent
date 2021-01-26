if (TARGET bencode::bencode)
    log_target_found(bencode)
    return()
endif()

find_package(bencode QUIET)
if (bencode_FOUND)
    log_found(bencode)
    return()
endif()

set(BENCODE_BUILD_TESTS      OFF)
set(BENCODE_BUILD_DOCS       OFF)
set(BENCODE_BUILD_BENCHMARKS OFF)

if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/bencode)
    log_dir_found(bencode)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/bencode)
    set(bencode_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/bencode)
else()
    log_fetch(bencode)
    FetchContent_Declare(
            bencode
            GIT_REPOSITORY https://github.com/fbdtemme/bencode.git
            GIT_TAG        master
    )
    set(BENCODE_BUILD_TESTS OFF)
    FetchContent_MakeAvailable(bencode)
endif()

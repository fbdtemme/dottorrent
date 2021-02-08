cmake_minimum_required(VERSION 3.15)

include(FetchContent)

function(log_target_found library)
    message(STATUS "Target of dependency ${library} already exist in project.")
endfunction()

function(log_module_found library)
    message(STATUS "Local installation of dependency ${library} found.")
endfunction()

function(log_dir_found library)
    message(STATUS "Source directory for dependency ${library} found.")
endfunction()

function(log_fetch library)
    message(STATUS "Fetching dependency ${library}...")
endfunction()


include(${CMAKE_CURRENT_LIST_DIR}/fmt.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/expected-lite.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/gsl-lite.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/mio.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/bencode.cmake)

if (DOTTORRENT_BUILD_TESTS)
    include(${CMAKE_CURRENT_LIST_DIR}/Catch2.cmake)
endif()


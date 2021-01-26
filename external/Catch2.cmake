if (TARGET Catch2::Catch2)
    log_target_found(Catch2)
    return()
endif()

find_package(Catch2 QUIET)
if (Catch2_FOUND)
    log_module_found(Catch2)
    return()
endif()

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/Catch2)
    log_dir_found(Catch2)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Catch2)
    set(Catch2_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/Catch2)
else()
    message(STATUS "Fetching dependency Catch2...")
    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG        v2.x
    )
    FetchContent_MakeAvailable(Catch2)
endif()

list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/contrib")

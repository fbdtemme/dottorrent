if (TARGET mio::mio)
    log_target_found(mio)
    return()
endif()

find_package(mio QUIET)
if (mio_FOUND)
    log_module_found(mio)
    return()
endif()

set(INSTALL_SUBPROJECTS ON CACHE STRING "Add mio to the install set" FORCE)

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/mio)
    log_dir_found(mio)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/mio)
    set(mio_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/mio)
else()
    log_fetch(mio)
    FetchContent_Declare(
            mio
            GIT_REPOSITORY https://github.com/mandreyel/mio.git
            GIT_TAG        master
    )
    FetchContent_MakeAvailable(mio)
endif()

if (IS_DIRECTORY ${mio_SOURCE_DIR})
    export(EXPORT mioTargets
           FILE mio-targets.cmake
           NAMESPACE mio::
    )
endif()



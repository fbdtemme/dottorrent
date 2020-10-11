cmake_minimum_required(VERSION 3.17)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

project(dottorrent
        DESCRIPTION "A C++20 torrent metafile library"
        VERSION 0.1.0
        LANGUAGES CXX
        )

include(CTest)
include(SanitizersConfig)
include(GNUInstallDirs)

option(DOTTORRENT_BUILD_TESTS      "Build tests" ON)
option(DOTTORRENT_BUILD_COVERAGE   "Enable coverage flags" OFF)
set(DOTTORRENT_CRYPTO_LIB "gcrypt" CACHE STRING
    "The cryptographic library to link agains. Options are: openssl, gcrypt"
)

# add cmake directory for Find* modules
cmake_policy(SET CMP0076 NEW)

add_library(dottorrent
        src/chunk_reader.cpp
        src/file_entry.cpp
        src/file_storage.cpp
        src/metafile.cpp
        src/metafile_parsing.cpp
        src/metafile_serialization.cpp
        src/storage_hasher.cpp
        src/v1_chunk_hasher.cpp
        src/v1_chunk_reader.cpp
        src/v1_chunk_verifier.cpp
        src/v2_chunk_reader.cpp
        src/v2_chunk_hasher.cpp
        src/hasher/backends/openssl.cpp
        src/hasher/backends/gcrypt.cpp
        )
add_library(dottorrent::dottorrent ALIAS dottorrent)

target_compile_features(dottorrent PUBLIC cxx_std_20)


string(TOLOWER ${DOTTORRENT_CRYPTO_LIB} DOTTORRENT_CRYPTO_LIB)

if (${DOTTORRENT_CRYPTO_LIB} STREQUAL "openssl")
    find_package(OpenSSL REQUIRED)
    target_link_libraries(${PROJECT_NAME}      PUBLIC OpenSSL::Crypto)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DOTTORRENT_USE_OPENSSL)

    message(STATUS "Using cryptographic library: OpenSSL")

elseif (${DOTTORRENT_CRYPTO_LIB} STREQUAL "gcrypt" )
    find_package(GCrypt REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC GCrypt::GCrypt)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DOTTORRENT_USE_GCRYPT)

    message(STATUS "Using cryptographic library: GCrypt")

else()
    message(FATAL_ERROR "Undefined crypto library: ${DOTTORRENT_CRYPTO_LIB}")
endif()

include(external/external.cmake)

find_package(Threads REQUIRED)
find_package(TBB REQUIRED)


target_include_directories(dottorrent PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        )

set(dottorrent_public_include_dir  ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME})


target_link_libraries(dottorrent
    PUBLIC
        Microsoft.GSL::GSL
        bencode::bencode
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

set(dottorrent_cmake_install_dir          ${CMAKE_INSTALL_LIBDIR}/cmake/dottorrent)
set(dottorrent_cmake_install_modules_dir  ${dottorrent_cmake_install_dir}/Modules)
set(dottorrent_version_config             ${PROJECT_BINARY_DIR}/dottorrent-config-version.cmake)
set(dottorrent_project_config             ${PROJECT_BINARY_DIR}/dottorrent-config.cmake)
set(dottorrent_targets_export_name        dottorrent-targets)
set(dottorrent_targets_file               ${dottorrent_targets_export_name}.cmake)
set(dottorrent_include_build_dir          ${PROJECT_SOURCE_DIR}/include/)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
        ${dottorrent_version_config}
        VERSION ${PACKAGE_VERSION}
        COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(
        ${PROJECT_SOURCE_DIR}/cmake/dottorrent-config.cmake.in
        ${dottorrent_project_config}
        INSTALL_DESTINATION ${dottorrent_cmake_install_dir})


# install project config file and config version file
install(FILES ${dottorrent_project_config}
        ${dottorrent_version_config}
        DESTINATION ${dottorrent_cmake_install_dir}
        )

# install FindTBB.cmake
install(FILES
        cmake/Modules/FindTBB.cmake
        DESTINATION ${dottorrent_cmake_install_modules_dir})

# install public headers
install(DIRECTORY ${dottorrent_public_include_dir} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# install targets to an export set
install(TARGETS dottorrent
        EXPORT ${dottorrent_targets_export_name}
        INCLUDES      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR})


if (NOT fmt_FOUND)
    message(STATUS "Adding fmt to install set")
    install(TARGETS fmt fmt-header-only
            EXPORT ${dottorrent_targets_export_name}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

endif()

if (NOT Microsoft.GSL_FOUND)
    message(STATUS "Adding Microsoft.GSL to install set")
    install(TARGETS GSL
            EXPORT ${dottorrent_targets_export_name}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

if (NOT bencode_FOUND)
    message(STATUS "Adding bencode to install set")
    install(TARGETS bencode
            EXPORT ${dottorrent_targets_export_name}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()


# Install the export set to enable importing targets from the build tree
export(EXPORT ${dottorrent_targets_export_name}
        FILE ${dottorrent_targets_file}
        NAMESPACE ${PROJECT_NAME}::)

# Install the export set to enable importing targets from the install tree
install(EXPORT ${dottorrent_targets_export_name}
        FILE ${dottorrent_targets_file}
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${dottorrent_cmake_install_dir})



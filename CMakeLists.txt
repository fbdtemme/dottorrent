cmake_minimum_required(VERSION 3.15)

project(dottorrent
        DESCRIPTION "A C++20 torrent metafile library"
        VERSION 0.4.0
        LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

include(CTest)
include(SanitizersConfig)
include(GNUInstallDirs)

option(DOTTORRENT_TESTS      "Build tests" ON)
option(DOTTORRENT_COVERAGE   "Enable coverage flags" OFF)
set(DOTTORRENT_CRYPTO_LIB "openssl" CACHE STRING
    "The cryptographic library to link against. Options are: openssl, wincng, wolfssl")
set(DOTTORRENT_MB_CRYPTO_LIB "openssl" CACHE STRING
        "The multibuffer cryptographic library to link against. Options are: none, isal")

option(DOTTORRENT_INSTALL            "Generate an install target" ON)

# add cmake directory for Find* modules
cmake_policy(SET CMP0076 NEW)

add_library(dottorrent
        src/announce_url_list.cpp
        src/chunk_hasher_multi_buffer.cpp
        src/chunk_hasher_single_buffer.cpp
        src/chunk_processor_base.cpp
        src/chunk_reader.cpp
        src/file_entry.cpp
        src/file_storage.cpp
        src/hasher/backends/gcrypt.cpp
        src/hasher/backends/isal.cpp
        src/hasher/backends/openssl.cpp
        src/hasher/backends/wincng.cpp
        src/hasher/backends/wolfssl.cpp
        src/magnet_uri.cpp
        src/metafile.cpp
        src/metafile_parsing.cpp
        src/metafile_serialization.cpp
        src/percent_encode.cpp
        src/storage_hasher.cpp
        src/storage_verifier.cpp
        src/v1_checksum_hasher.cpp
        src/v1_chunk_hasher_mb.cpp
        src/v1_chunk_hasher_sb.cpp
        src/v1_chunk_reader.cpp
        src/v1_piece_verifier.cpp
        src/v1_piece_writer.cpp
        src/v2_checksum_hasher.cpp
        src/v2_chunk_hasher_mb.cpp
        src/v2_chunk_hasher_mixin.cpp
        src/v2_chunk_hasher_sb.cpp
        src/v2_chunk_hasher_sb.cpp
        src/v2_chunk_reader.cpp
        src/v2_piece_verifier.cpp
        src/v2_piece_writer.cpp
)


add_library(dottorrent::dottorrent ALIAS dottorrent)
target_compile_features(dottorrent PUBLIC cxx_std_20)

include(external/external.cmake)

if (WIN32)
    message(STATUS "Windows platform detected: Setting default crypto backend to WinCNG")
    set(DOTTORRENT_CRYPTO_LIB "WinCNG")
endif()

string(TOLOWER ${DOTTORRENT_CRYPTO_LIB} DOTTORRENT_CRYPTO_LIB)
string(TOLOWER ${DOTTORRENT_MB_CRYPTO_LIB} DOTTORRENT_MB_CRYPTO_LIB)

if (DOTTORRENT_CRYPTO_LIB STREQUAL "openssl")
    find_package(OpenSSL REQUIRED)
    target_link_libraries(${PROJECT_NAME}      PUBLIC OpenSSL::Crypto)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DOTTORRENT_USE_OPENSSL)
    message(STATUS "Using cryptographic library: OpenSSL")

elseif(DOTTORRENT_CRYPTO_LIB STREQUAL "wincng")
    target_link_libraries(${PROJECT_NAME}   PUBLIC bcrypt)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DOTTORRENT_USE_WINCNG)
    message(STATUS "Using cryptographic library: WinCNG")

elseif(DOTTORRENT_CRYPTO_LIB STREQUAL "wolfssl")
    include(${CMAKE_CURRENT_LIST_DIR}/external/wolfSSL.cmake)
    target_link_libraries(${PROJECT_NAME} PUBLIC wolfssl)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DOTTORRENT_USE_WOLFSSL)
    message(STATUS "Using cryptographic library: wolfSSL")
else()
    message(FATAL_ERROR "Unrecognised crypto library: ${DOTTORRENT_CRYPTO_LIB}")
endif()

if (DOTTORRENT_MB_CRYPTO_LIB STREQUAL "isal")
    include(${CMAKE_CURRENT_LIST_DIR}/external/isa-l_crypto.cmake)
    target_link_libraries(${PROJECT_NAME}        PUBLIC ISAL::Crypto)
    get_property(dirs TARGET ${PROJECT_NAME} PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    target_compile_definitions(${PROJECT_NAME}   PUBLIC DOTTORRENT_USE_ISAL)
    message(STATUS "Using multibuffer cryptographic library: Intel ISA-L")
endif()

find_package(Threads REQUIRED)

target_include_directories(dottorrent PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        )

set(dottorrent_public_include_dir  ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME})

target_link_libraries(dottorrent
    PUBLIC
        bencode::bencode
        fmt::fmt-header-only
        gsl::gsl-lite-v1
        Threads::Threads
)

if (DOTTORRENT_BUILD_COVERAGE)
    message(STATUS "Building with coverage enabled")
    set(CMAKE_CXX_FLAGS --coverage)
    set(CMAKE_C_FLAGS --coverage)
endif()

if (DOTTORRENT_TESTS)
add_subdirectory(tests)
endif()

if (DOTTORRENT_INSTALL)
    set(dottorrent_cmake_install_dir          ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
    set(dottorrent_cmake_install_modules_dir  ${dottorrent_cmake_install_dir}/Modules)
    set(dottorrent_version_config             ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake)
    set(dottorrent_project_config             ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake)
    set(dottorrent_targets_export_name        ${PROJECT_NAME}-targets)
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

    # install public headers
    install(DIRECTORY ${dottorrent_public_include_dir} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    # install targets to an export set
    install(TARGETS dottorrent
            EXPORT   ${dottorrent_targets_export_name}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR})

    # Install the export set to ena ble importing targets from the build tree
    export(EXPORT ${dottorrent_targets_export_name}
            FILE ${dottorrent_targets_file}
            NAMESPACE ${PROJECT_NAME}::)

    # Install the export set to enable importing targets from the install tree
    install(EXPORT ${dottorrent_targets_export_name}
            FILE ${dottorrent_targets_file}
            NAMESPACE ${PROJECT_NAME}::
            DESTINATION ${dottorrent_cmake_install_dir})
endif()

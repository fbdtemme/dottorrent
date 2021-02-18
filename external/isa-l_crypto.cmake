 include(ExternalProject)
 ExternalProject_Add(isa-l_crypto-populate
     GIT_REPOSITORY "https://github.com/intel/isal-crypto.git"
     GIT_TAG "master"
     SOURCE_DIR          "${CMAKE_CURRENT_BINARY_DIR}/_deps/isa-l_crypto-src"
     SOURCE_DIR          "${CMAKE_CURRENT_BINARY_DIR}/_deps/isa-l_crypto-build"
     CONFIGURE_COMMAND   ""
     BUILD_COMMAND       ""
     INSTALL_COMMAND     ""
     TEST_COMMAND        ""
     USES_TERMINAL_DOWNLOAD  YES
     USES_TERMINAL_UPDATE    YES
 )

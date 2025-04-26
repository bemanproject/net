# cmake-format: off
# /CMakeLists.txt -*-makefile-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# cmake-format: on

cmake_minimum_required(VERSION 3.23)
project(
  beman_net
  VERSION 0.0.0
  LANGUAGES CXX)
set(TARGET_NAME net)
set(TARGET_PREFIX beman.${TARGET_NAME})
set(TARGET_LIBRARY beman_${TARGET_NAME})
set(TARGET_ALIAS beman::${TARGET_NAME})
set(TARGETS_EXPORT_NAME ${CMAKE_PROJECT_NAME})

set(CMAKE_CXX_STANDARD 23)

include(CheckIncludeFileCXX)
include(CheckCXXSourceCompiles)
check_include_file_cxx(sys/event.h NET_HAS_KQUEUE)
if(NET_HAS_KQUEUE)
  add_compile_definitions(NET_HAS_KQUEUE)
  check_cxx_source_compiles(
    "
    #include <sys/event.h>
    #include <stdint.h>
    int main() {
        struct kevent ev{::uintptr_t(), ::int16_t(), EV_DELETE, ::uint32_t(), ::intptr_t(), nullptr, {}};
        (void)ev;
    }
    "
    NET_KEVENT_HAS_EXT)
  if(NET_KEVENT_HAS_EXT)
    add_compile_definitions(NET_KEVENT_HAS_EXT)
  endif()
endif()

include(FetchContent)
FetchContent_Declare(
  execution
  # for local development, use SOURCE_DIR <path-to>/execution
  GIT_REPOSITORY https://github.com/bemanproject/execution
  GIT_TAG 9cd2e66ba312c4396ac222868cc0486891cfa30b)
FetchContent_MakeAvailable(execution)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests/beman/${TARGET_NAME})
  enable_testing()
endif()

add_subdirectory(src/beman/${TARGET_NAME})
add_subdirectory(examples)

include(GNUInstallDirs)

set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion)

configure_package_config_file(
  "cmake/Config.cmake.in"
  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION ${INSTALL_CONFIGDIR})

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ConfigVersion.cmake
  DESTINATION ${INSTALL_CONFIGDIR})

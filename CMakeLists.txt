# cmake-format: off
# /CMakeLists.txt -*-makefile-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# cmake-format: on

cmake_minimum_required(VERSION 3.30...4.3)

project(beman.net VERSION 0.1.0 LANGUAGES CXX)

# [CMAKE.SKIP_TESTS]
option(
    BEMAN_NET_BUILD_TESTS
    "Enable building tests and test infrastructure. Default: ON. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

# [CMAKE.SKIP_EXAMPLES]
option(
    BEMAN_NET_BUILD_EXAMPLES
    "Enable building examples. Default: ON. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

if(LINUX)
    option(BEMAN_NET_WITH_URING "Enable liburing io context" OFF)
endif()

include(FetchContent)

FetchContent_Declare(
    execution
    # FETCHCONTENT_SOURCE_DIR_EXECUTION ${CMAKE_SOURCE_DIR}/../execution
    GIT_REPOSITORY https://github.com/bemanproject/execution
    GIT_TAG 66295d5
    SYSTEM
    FIND_PACKAGE_ARGS
        0.2.0
        EXACT
        NAMES
        beman.execution
        COMPONENTS
        execution_headers
)
FetchContent_MakeAvailable(execution)

FetchContent_Declare(
    task
    # FETCHCONTENT_SOURCE_DIR_TASK ${CMAKE_SOURCE_DIR}/../task
    GIT_REPOSITORY https://github.com/bemanproject/task
    GIT_TAG 9075137
    FIND_PACKAGE_ARGS 0.2.0 EXACT NAMES beman.task COMPONENTS task_headers
)
FetchContent_MakeAvailable(task)

add_subdirectory(src/beman/net)

#===============================================================================
# NOTE: this must be done before tests! CK
include(./cmake/beman-install-library.cmake)
beman_install_library(${PROJECT_NAME} TARGETS beman.net_headers
    DEPENDENCIES [===[beman.task 0.2.0 EXACT]===]
)
#===============================================================================

enable_testing()

if(BEMAN_NET_BUILD_TESTS)
    add_subdirectory(tests/beman/net)
endif()

if(BEMAN_NET_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

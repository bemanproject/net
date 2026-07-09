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
    task
    #SOURCE_DIR ${CMAKE_SOURCE_DIR}/../task
    GIT_REPOSITORY https://github.com/bemanproject/task
    GIT_TAG 1679185
    FIND_PACKAGE_ARGS 0.2.0 EXACT NAMES beman.task COMPONENTS task_headers
)
FetchContent_MakeAvailable(task)

# MSVC C7760 workaround for execution 7df0f75 (pre-existing on main).
# Upstream fix: bemanproject/execution@f131d69. Remove when task updates.
FetchContent_GetProperties(
    execution
    POPULATED _beman_execution_populated
    SOURCE_DIR _beman_execution_source_dir
)
if(_beman_execution_populated)
    set(_beman_execution_product_type_cppm
        "${_beman_execution_source_dir}/src/beman/execution/product_type.cppm"
    )
    if(EXISTS "${_beman_execution_product_type_cppm}")
        file(
            READ "${_beman_execution_product_type_cppm}"
            _beman_execution_product_type_content
        )
        if(
            _beman_execution_product_type_content
                MATCHES
                "export template <typename... T>"
        )
            string(
                REPLACE "#ifdef _MSC_VER"
                "#if defined(_MSC_VER) && _MSC_VER <= 1944L"
                _beman_execution_product_type_content
                "${_beman_execution_product_type_content}"
            )
            string(
                REPLACE "export template <typename... T>"
                "template <typename... T>"
                _beman_execution_product_type_content
                "${_beman_execution_product_type_content}"
            )
            string(
                REPLACE "export template <::std::size_t I, typename... T>"
                "template <::std::size_t I, typename... T>"
                _beman_execution_product_type_content
                "${_beman_execution_product_type_content}"
            )
            file(
                WRITE "${_beman_execution_product_type_cppm}"
                "${_beman_execution_product_type_content}"
            )
        endif()
    endif()
endif()

include(CheckLibraryExists)
set(POSTGRESROOT /Library/PostgreSQL/18)
check_library_exists(pq PQsocket ${POSTGRESROOT}/lib HAS_POSTGRES)

add_subdirectory(src/beman/net)
add_library(beman::net ALIAS beman.net)

#===============================================================================
# NOTE: this must be done before tests! CK
include(infra/cmake/beman-install-library.cmake)
beman_install_library(${PROJECT_NAME} TARGETS beman.net_headers
    DEPENDENCIES [===[beman.task 0.2.0 EXACT]===]
)
#===============================================================================

enable_testing()

if(BEMAN_NET_BUILD_TESTS)
    add_subdirectory(tests/beman/net)
endif()

if(${BEMAN_NET_BUILD_EXAMPLES})
    message(NOTICE "Building examples")
    add_subdirectory(examples)
endif()

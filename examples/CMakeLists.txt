# cmake-format: off
# examples/CMakeLists.txt -*-makefile-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# cmake-format: on

cmake_minimum_required(VERSION 3.30...4.3)

# include(../cmake/prelude.cmake)

project(beman_net.example LANGUAGES CXX)

# TODO(CK): cmake -S . -G Ninja -B build -D CMAKE_BUILD_TYPE=RelWithDebInfo --fresh
if(PROJECT_IS_TOP_LEVEL)
    # include(../cmake/cxx-modules-rules.cmake)
    set(BEMAN_USE_MODULES OFF)
    set(BEMAN_USE_STD_MODULE OFF)
    set(CMAKE_CXX_STANDARD 23)

    find_package(beman.net 0.1.0 EXACT REQUIRED)

    enable_testing()
endif()

set(EXAMPLES
    http-warwick
    client
    cppcon-2024
    empty
    http-server
    milano
    http-munich
    postgres
    server
    taps
    task
)
set(xEXAMPLES taps)
set(EXAMPLES postgres populate-postgres)

foreach(EXAMPLE ${EXAMPLES})
    set(EXAMPLE_TARGET beman.net.examples.${EXAMPLE})
    add_executable(${EXAMPLE_TARGET})
    if(BEMAN_NET_WITH_URING)
        # XXX target_compile_definitions( ${EXAMPLE_TARGET} PRIVATE BEMAN_NET_USE_URING)
    endif()
    target_sources(${EXAMPLE_TARGET} PRIVATE ${EXAMPLE}.cpp)
    target_include_directories(${EXAMPLE_TARGET} PRIVATE /Library/PostgreSQL/18/include)
    target_link_directories(${EXAMPLE_TARGET} PRIVATE /Library/PostgreSQL/18/lib)
    target_link_libraries(${EXAMPLE_TARGET} PRIVATE pq)
    target_link_libraries(${EXAMPLE_TARGET} PRIVATE beman::net_headers)
    target_link_libraries(${EXAMPLE_TARGET} PRIVATE beman::task_headers)
endforeach()

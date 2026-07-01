# cmake-format: off
# src/beman/net/CMakeLists.txt -*-makefile-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# cmake-format: on

add_library(beman.sequence_headers INTERFACE)
add_library(beman::sequence_headers ALIAS beman.sequence_headers)

target_sources(
    beman.sequence_headers
    # XXX PRIVATE sequence.cpp
    PUBLIC
        FILE_SET public_headers
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}/include
            FILES
                ${PROJECT_SOURCE_DIR}/include/beman/sequence/sequence.hpp
    PUBLIC
        FILE_SET detail_headers
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}/include
            FILES
                ${PROJECT_SOURCE_DIR}/include/beman/sequence/detail/ignore_all.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/sequence/detail/iota.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/sequence/detail/sequence_sender.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/sequence/detail/set_next.hpp
)

get_property(
    DETAIL_HEADER_FILES
    TARGET beman.sequence_headers
    PROPERTY HEADER_SET_detail_headers
)
source_group("Header Files\\detail" FILES ${DETAIL_HEADER_FILES})

target_link_libraries(beman.sequence_headers INTERFACE beman::task_headers)
set_target_properties(
    beman.sequence_headers
    PROPERTIES
        VERIFY_INTERFACE_HEADER_SETS
            OFF # FIXME: ${PROJECT_IS_TOP_LEVEL}
)

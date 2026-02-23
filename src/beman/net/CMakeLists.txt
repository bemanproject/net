# cmake-format: off
# src/beman/net/CMakeLists.txt -*-makefile-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# cmake-format: on

add_library(${PROJECT_NAME}_headers INTERFACE)
add_library(beman::net_headers ALIAS ${PROJECT_NAME}_headers)

target_sources(
    ${PROJECT_NAME}_headers
    # XXX PRIVATE net.cpp
    PUBLIC
        FILE_SET public_headers
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}/include
            FILES
                ${PROJECT_SOURCE_DIR}/include/beman/net/net.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net29/net.hpp
    PUBLIC
        FILE_SET detail_headers
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}/include
            FILES
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/into_expected.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/netfwd.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/poll_context.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/container.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/basic_stream_socket.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/security_props.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/io_context_scheduler.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/internet.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/transport_preference.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/transport_props.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/scope.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/execution.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/get_io_handle.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/context_base.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/task.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/preconnection.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/local_endpoint.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/rendezvous.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/io_context.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/basic_socket.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/timer.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/io_base.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/endpoint.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/repeat_effect_until.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/basic_socket_acceptor.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/socket_category.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/socket_base.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/listen.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/event_type.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/operations.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/buffer.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/stop_token.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/sorted_list.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/sender.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/get_scope_token.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/remote_endpoint.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/uring_context.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/net/detail/initiate.hpp
)

get_property(
    DETAIL_HEADER_FILES
    TARGET ${PROJECT_NAME}_headers
    PROPERTY HEADER_SET_detail_headers
)
source_group("Header Files\\detail" FILES ${DETAIL_HEADER_FILES})

target_link_libraries(${PROJECT_NAME}_headers INTERFACE beman::task_headers)
set_target_properties(
    ${PROJECT_NAME}_headers
    PROPERTIES
        VERIFY_INTERFACE_HEADER_SETS
            OFF # FIXME: ${PROJECT_IS_TOP_LEVEL}
)

if(LINUX AND BEMAN_NET_WITH_URING)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(uring REQUIRED IMPORTED_TARGET liburing)
    target_link_libraries(${PROJECT_NAME}_headers INTERFACE PkgConfig::uring)
    target_compile_definitions(
        ${PROJECT_NAME}_headers
        INTERFACE BEMAN_NET_USE_URING
    )
endif()

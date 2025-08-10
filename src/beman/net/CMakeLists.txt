# cmake-format: off
# src/beman/net/CMakeLists.txt -*-makefile-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# cmake-format: on

add_library(${TARGET_LIBRARY} STATIC)
add_library(${TARGET_ALIAS} ALIAS ${TARGET_LIBRARY})

target_sources(${TARGET_LIBRARY}
    PRIVATE
    net.cpp
    PUBLIC
    FILE_SET ${TARGET_LIBRARY}_public_headers TYPE HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}/include
    FILES
    ${PROJECT_SOURCE_DIR}/include/beman/net/net.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net29/net.hpp
    PUBLIC
    FILE_SET ${TARGET_LIBRARY}_detail_headers TYPE HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}/include
    FILES
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/basic_socket.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/basic_socket_acceptor.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/basic_stream_socket.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/buffer.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/container.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/context_base.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/endpoint.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/event_type.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/execution.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/get_io_handle.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/get_scope_token.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/initiate.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/internet.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/into_expected.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/io_base.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/io_context.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/io_context_scheduler.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/listen.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/local_endpoint.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/netfwd.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/operations.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/poll_context.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/preconnection.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/remote_endpoint.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/rendezvous.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/repeat_effect_until.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/scope.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/security_props.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/sender.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/socket_base.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/socket_category.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/sorted_list.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/stop_token.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/task.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/timer.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/transport_preference.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/net/detail/transport_props.hpp
)
get_property(DETAIL_HEADER_FILES TARGET ${TARGET_LIBRARY} PROPERTY HEADER_SET_${TARGET_LIBRARY}_detail_headers)
source_group("Header Files\\detail" FILES ${DETAIL_HEADER_FILES})

include(GNUInstallDirs)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

string(TOLOWER ${CMAKE_PROJECT_NAME} CMAKE_LOWER_PROJECT_NAME)

install(
  TARGETS ${TARGET_LIBRARY}
  EXPORT ${TARGETS_EXPORT_NAME}
  ARCHIVE DESTINATION lib/$<CONFIG>
  FILE_SET ${TARGET_LIBRARY}_public_headers
  FILE_SET ${TARGET_LIBRARY}_detail_headers
)
target_include_directories(${TARGET_LIBRARY} PUBLIC $<INSTALL_INTERFACE:include>)
target_link_libraries(${TARGET_LIBRARY} PUBLIC beman::task)

if(FALSE)
install(EXPORT ${TARGETS_EXPORT_NAME}
  FILE ${TARGET_LIBRARY}-config.cmake
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${TARGET_LIBRARY}"
  NAMESPACE ${TARGET_LIBRARY}::
)
endif()

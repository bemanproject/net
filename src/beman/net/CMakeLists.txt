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
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/net.hpp
    PUBLIC
    FILE_SET ${TARGET_LIBRARY}_detail_headers TYPE HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}/include
    FILES
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/basic_socket.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/basic_socket_acceptor.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/basic_stream_socket.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/buffer.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/container.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/context_base.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/endpoint.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/execution.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/internet.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/io_base.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/io_context.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/io_context_scheduler.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/netfwd.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/operations.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/poll_context.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/sender.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/socket_base.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/socket_category.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/sorted_list.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/stop_token.hpp
    ${PROJECT_SOURCE_DIR}/include/beman/${TARGET_NAME}/detail/timer.hpp
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

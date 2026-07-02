# cmake-format: off
# src/beman/net/CMakeLists.txt -*-makefile-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# cmake-format: on

list(APPEND tests
    connector.pass
    ignore_all.pass
    iota.pass
    sequence_sender.pass
    set_next.pass
)

foreach(test ${tests})
    add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} PRIVATE beman::net_headers beman::sequence_headers)
    add_test(NAME ${test} COMMAND $<TARGET_FILE:${test}>)
endforeach()

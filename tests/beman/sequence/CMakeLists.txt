# cmake-format: off
# src/beman/net/CMakeLists.txt -*-makefile-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# cmake-format: on

list(APPEND tests
    connector.test
    filter_each.test
    ignore_all.test
    iota.test
    sequence_connect.test
    sequence_connect_result_t.test
    sequence_receiver.test
    sequence_sender.test
    set_next.test
    then_each.test
)

foreach(test ${tests})
    add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} PRIVATE beman::net_headers beman::sequence_headers)
    add_test(NAME ${test} COMMAND $<TARGET_FILE:${test}>)
endforeach()

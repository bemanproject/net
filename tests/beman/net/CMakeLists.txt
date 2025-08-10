# cmake-format: off
# src/beman/net/CMakeLists.txt -*-makefile-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# cmake-format: on

list(APPEND tests sorted_list.pass)

foreach(test ${tests})
    add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} PRIVATE ${TARGET_LIBRARY})
    add_test(NAME ${test} COMMAND $<TARGET_FILE:${test}>)
endforeach()

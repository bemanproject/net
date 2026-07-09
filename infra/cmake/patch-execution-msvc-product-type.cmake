# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.24)

if(NOT FILE)
    message(FATAL_ERROR "FILE must be set")
endif()

if(NOT EXISTS "${FILE}")
    message(FATAL_ERROR "File not found: ${FILE}")
endif()

file(READ "${FILE}" _content)
if(NOT _content MATCHES "export template <typename... T>")
    return()
endif()

string(
    REPLACE "#ifdef _MSC_VER"
    "#if defined(_MSC_VER) && _MSC_VER <= 1944L"
    _content
    "${_content}"
)
string(
    REPLACE "export template <typename... T>"
    "template <typename... T>"
    _content
    "${_content}"
)
string(
    REPLACE "export template <::std::size_t I, typename... T>"
    "template <::std::size_t I, typename... T>"
    _content
    "${_content}"
)
file(WRITE "${FILE}" "${_content}")

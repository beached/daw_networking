cmake_minimum_required(VERSION 3.16)
project(daw_networking CXX)

set(CMAKE_CXX_STANDARD 17)

include_directories(include/)

include_directories(include/)
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    include_directories(.glean/release/include/)
else ()
    include_directories(.glean/debug/include/)
endif ()


add_library(daw_tcp_client SHARED src/tcp_client.cpp src/async_exec_policy_thread.cpp)

add_executable(tcp_client_test_bin tests/tcp_client_test.cpp)
target_link_libraries(tcp_client_test_bin daw_tcp_client)
add_test(tcp_client_test tcp_client_test_bin)
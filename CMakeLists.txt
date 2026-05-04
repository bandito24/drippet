
cmake_minimum_required(VERSION 3.16)
# Enable coverage flags in Debug mode or when a COVERAGE option is enabled
option(COVERAGE "Enable coverage reporting" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(drippet)
set (CMAKE_CXX_STANDARD 20)

SET(NODE_CORE_DIR "drippet_node/main/core")
SET(HEAD_CORE_DIR "drippet_head/main/core")

add_subdirectory(shared)
add_subdirectory(${NODE_CORE_DIR})
add_subdirectory(${HEAD_CORE_DIR})
add_subdirectory(tests)
add_executable(boop boop.cpp)
target_link_libraries(boop PUBLIC SHARED_DPT NODE_SRC HEAD_SRC)

####*###


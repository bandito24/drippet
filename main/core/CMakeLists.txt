
cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(waterfall)
set (CMAKE_CXX_STANDARD 20)

add_subdirectory(tests)
add_subdirectory(src)


add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC SRC)



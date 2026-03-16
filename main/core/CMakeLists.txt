
cmake_minimum_required(VERSION 3.16)
# Enable coverage flags in Debug mode or when a COVERAGE option is enabled
option(COVERAGE "Enable coverage reporting" ON)



set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(waterfall)
set (CMAKE_CXX_STANDARD 20)


add_subdirectory(src)
add_subdirectory(tests)



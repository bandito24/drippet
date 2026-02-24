file(GLOB source_files *.cpp)
add_library(SRC ${source_files} "../env/host/time.cpp")
target_include_directories(SRC PUBLIC ${PROJECT_SOURCE_DIR}/include)


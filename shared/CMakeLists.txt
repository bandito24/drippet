file(GLOB SOURCE_FILES src/*.cpp env/host/*.cpp)
add_library(SHARED_DPT ${SOURCE_FILES})
target_include_directories(SHARED_DPT PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

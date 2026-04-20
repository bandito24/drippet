file(GLOB SOURCE_FILES host/*.cpp)
add_library(SHARED_DPT ${SOURCE_FILES})
target_include_directories(SHARED_DPT PUBLIC include)

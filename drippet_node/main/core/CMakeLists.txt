
file(GLOB source_files ./src/*.cpp)
add_library(NODE_SRC ${source_files})


target_link_libraries(NODE_SRC PUBLIC SHARED_DPT)
target_include_directories(NODE_SRC PUBLIC
${CMAKE_CURRENT_SOURCE_DIR}/include
#${PROJECT_SOURCE_DIR}/shared/include
)
####*####


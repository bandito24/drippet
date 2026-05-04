
file(GLOB source_files CONFIGURE_DEPENDS src/*.cpp)
add_library(HEAD_SRC ${source_files})

target_link_libraries(HEAD_SRC PUBLIC SHARED_DPT)
target_include_directories(HEAD_SRC PUBLIC
${CMAKE_CURRENT_SOURCE_DIR}/include
${PROJECT_SOURCE_DIR}/shared/include

)
####*###


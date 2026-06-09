
file(GLOB source_files CONFIGURE_DEPENDS src/*.cpp)

file(GLOB host_files CONFIGURE_DEPENDS env/host/*.cpp)
add_library(HEAD_SRC ${source_files} ${host_files})

target_link_libraries(HEAD_SRC PUBLIC)
target_include_directories(HEAD_SRC PUBLIC
${CMAKE_CURRENT_SOURCE_DIR}/include
)
####*###


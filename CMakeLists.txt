file(GLOB source_files *.cpp)
set(ABS_PROJ_ROOT "Users/char/esp/drippet")
add_library(SRC ${source_files} "../env/host/logger.cpp")
target_include_directories(SRC PUBLIC ${PROJECT_SOURCE_DIR}/include ${ABS_PROJ_ROOT}/shared/include)
####*###


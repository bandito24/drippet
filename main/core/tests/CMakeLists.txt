include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.8.1
)

FetchContent_Declare(
  fakeit
  GIT_REPOSITORY https://github.com/eranpeer/FakeIt.git
  GIT_TAG master
)

FetchContent_MakeAvailable(Catch2 fakeit)

file(GLOB TEST_FILES *_test.cpp)

add_executable(tests ${TEST_FILES})

target_link_libraries(tests
    PRIVATE
    SRC
    Catch2::Catch2WithMain
)

target_include_directories(tests PRIVATE
    ${fakeit_SOURCE_DIR}/single_header/catch
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(tests PRIVATE
    PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

# Coverage instrumentation
if(COVERAGE)
  target_compile_options(SRC PRIVATE
        -fprofile-instr-generate
        -fcoverage-mapping
    )

  target_compile_options(tests PRIVATE
        -fprofile-instr-generate
        -fcoverage-mapping
    )

  target_link_options(tests PRIVATE
        -fprofile-instr-generate
    )
endif()

include(CTest)
include(Catch)
catch_discover_tests(tests)

# Coverage files
set(PROFRAW_TESTS "tests.profraw")
set(PROFDATA_FILE "coverage.profdata")

add_custom_target(run_tests
    COMMAND LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/${PROFRAW_TESTS} ./tests
    DEPENDS tests
    COMMENT "Running tests to generate ${PROFRAW_TESTS}"
)

add_custom_target(merge_coverage
    COMMAND llvm-profdata merge
        -sparse
        ${CMAKE_BINARY_DIR}/${PROFRAW_TESTS}
        -o ${CMAKE_BINARY_DIR}/${PROFDATA_FILE}
    DEPENDS run_tests
)

add_custom_target(coverage_report
    COMMAND llvm-cov show ./tests
        -instr-profile=${CMAKE_BINARY_DIR}/${PROFDATA_FILE}
        -format=html
        -output-dir=${CMAKE_BINARY_DIR}/coverage_report
        -ignore-filename-regex="_deps|tests|/usr/include"
    DEPENDS merge_coverage
)

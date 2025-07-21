include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
  safebuff_test
  ./unit/safebuff_test.cc
)
target_link_libraries(
  safebuff_test
  dat_lib
  GTest::gtest_main
)
target_include_directories(safebuff_test PRIVATE ${CMAKE_SOURCE_DIR}/custom/inc)

include(GoogleTest)
gtest_discover_tests(safebuff_test)
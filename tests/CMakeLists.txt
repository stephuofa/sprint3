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
  all_tests
  ./unit/all_tests.cc
  ./unit/safebuff_tests.cc
  ./unit/dataprocessor_tests.cc
)
target_link_libraries(
  all_tests
  dat_lib
  log_lib
  GTest::gtest_main
)
target_include_directories(all_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/unit)

include(GoogleTest)
gtest_discover_tests(all_tests)
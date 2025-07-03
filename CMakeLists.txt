cmake_minimum_required(VERSION 3.21)

project(Sprint3 C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD   17)

message("build type: ${CMAKE_BUILD_TYPE}")
option(MAKE_TESTS "Build tests" OFF)
option(MAKE_MIN "Make min" OFF)

# Output dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/arch)
 
# Compiler flags
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -pedantic)
endif()

# build katherine
add_subdirectory(katherine)




if(MAKE_MIN)
  add_executable(minEx core/minimalEx.cpp)
  target_link_libraries(minEx PRIVATE katherinexx)
else()
  # Sources as libraries
  add_library(acq_lib STATIC ${PROJECT_SOURCE_DIR}/custom/src/AcqController.cpp)
  target_include_directories(acq_lib PUBLIC ./custom/inc)
  target_link_libraries(acq_lib PUBLIC katherinexx)

  add_library(dat_lib STATIC ${PROJECT_SOURCE_DIR}/custom/src/DataProcessor.cpp)
  target_include_directories(dat_lib PUBLIC ./custom/inc)
  target_link_libraries(dat_lib PUBLIC katherinexx)


  add_library(str_lib STATIC ${PROJECT_SOURCE_DIR}/custom/src/StorageManager.cpp)
  target_include_directories(str_lib PUBLIC ./custom/inc)
  target_link_libraries(str_lib PUBLIC katherinexx)


  add_library(log_lib STATIC ${PROJECT_SOURCE_DIR}/custom/src/Logger.cpp)
  target_include_directories(log_lib PUBLIC ./custom/inc)
  target_link_libraries(str_lib PUBLIC katherinexx)

  add_executable(sprint core/main.cpp)
  target_link_libraries(sprint PRIVATE acq_lib dat_lib str_lib log_lib)
endif()



if(MAKE_TESTS)
  message(STATUS "Building tests")
  add_subdirectory(tests)
endif()
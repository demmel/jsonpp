cmake_minimum_required(VERSION 3.25.1)

project(json++ VERSION 0.1 LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)



# Include directories
include_directories(include)

# Add library
add_library(lib STATIC src/lib.cpp)

if(MSVC)
  target_compile_options(lib PRIVATE /W4)
else()
  target_compile_options(lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Test Deps
find_package(GTest REQUIRED)

# Add test executable
add_executable(test_lib tests/test_lib.cpp)
target_link_libraries(test_lib gtest::gtest GTest::gtest_main lib)

# Discover tests
enable_testing()
include(GoogleTest)
gtest_discover_tests(test_lib)
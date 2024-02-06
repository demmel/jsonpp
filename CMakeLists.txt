cmake_minimum_required(VERSION 3.25.1)

project(json++ VERSION 0.1 LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(include)
include_directories("${CMAKE_INCLUDE_PATH}")

# Add library
add_library(lib STATIC src/lib.cpp)

# Test Deps
find_package(GTest REQUIRED)

# Add test executable
add_executable(test_lib tests/test_lib.cpp)
target_link_libraries(test_lib gtest::gtest GTest::gtest_main lib)

# Discover tests
enable_testing()
include(GoogleTest)
gtest_discover_tests(test_lib)
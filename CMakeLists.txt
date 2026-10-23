cmake_minimum_required(VERSION 3.16)
project(lru_cache CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(USE_TSAN "Build with ThreadSanitizer" OFF)
if(USE_TSAN)
    add_compile_options(-fsanitize=thread -g)
    add_link_options(-fsanitize=thread)
endif()

find_package(Threads REQUIRED)

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark PRIVATE Threads::Threads)
target_compile_options(benchmark PRIVATE -UNDEBUG)

find_package(GTest)
if(GTest_FOUND)
    enable_testing()
    add_executable(run_tests test_cache.cpp)
    target_link_libraries(run_tests PRIVATE GTest::gtest_main Threads::Threads)
    add_test(NAME run_tests COMMAND run_tests)
endif()

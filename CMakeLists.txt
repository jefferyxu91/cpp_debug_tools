cmake_minimum_required(VERSION 3.10)
project(debug_containers VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Include directories
include_directories(include)
include_directories(thirdparty/nanoflann/include)

# Find GTest (optional)
include(FetchContent)
set(BUILD_GTEST OFF CACHE BOOL "")
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    message(STATUS "GoogleTest not found locally. Fetching via FetchContent...")
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    # targets GTest::gtest and GTest::gtest_main are provided by googletest project
    set(GTest_FOUND ON)
endif()

# Find ROS if available (optional)
find_package(rosconsole QUIET)

# Google Test executable (only if GTest is found)
if(GTest_FOUND)
    add_executable(debug_containers_test test/debug_containers_test.cpp)
    target_link_libraries(debug_containers_test GTest::gtest GTest::gtest_main)

    add_executable(watcher_test test/nanoflann_watcher_test.cpp)
    target_link_libraries(watcher_test GTest::gtest GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(debug_containers_test)
    gtest_discover_tests(watcher_test)
endif()

# Example executable
add_executable(memory_debug_example examples/memory_debug_example.cpp)
add_executable(nanoflann_memory_watch_example examples/nanoflann_memory_watch_example.cpp)

# Set compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(GTest_FOUND)
        target_compile_options(debug_containers_test PRIVATE -Wall -Wextra -O2)
    endif()
    target_compile_options(memory_debug_example PRIVATE -Wall -Wextra -O2)
    target_compile_options(nanoflann_memory_watch_example PRIVATE -Wall -Wextra -O2)
endif()

# Install rules
install(DIRECTORY include/memory/container_debug/
        DESTINATION include/memory/container_debug/
        FILES_MATCHING PATTERN "*.hpp")

install(FILES docs/MEMORY_DEBUG_GUIDE.md docs/ROS_INTEGRATION_GUIDE.md
        DESTINATION share/debug_containers/docs/)

# Print configuration summary
message(STATUS "Debug Containers Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  GTest: ${GTest_FOUND}")
message(STATUS "  ROS Integration: ${rosconsole_FOUND}")
message(STATUS "  Executables to build:")
if(GTest_FOUND)
    message(STATUS "    - debug_containers_test (Google Test)")
    message(STATUS "    - watcher_test (Google Test)")
endif()
message(STATUS "    - memory_debug_example")
message(STATUS "    - nanoflann_memory_watch_example")
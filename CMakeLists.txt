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
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(WARNING "GTest not found. Tests will not be built.")
    message(STATUS "To install GTest: sudo apt-get install libgtest-dev")
endif()

# Find ROS if available (optional)
find_package(rosconsole QUIET)

# Google Test executable (only if GTest is found)
if(GTest_FOUND)
    add_executable(debug_containers_test test/debug_containers_test.cpp)
    target_link_libraries(debug_containers_test GTest::gtest GTest::gtest_main)
endif()

# Example executables
add_executable(memory_debug_example examples/memory_debug_example.cpp)
add_executable(simple_integration_example examples/simple_integration_example.cpp)
add_executable(monitored_kdtree_example examples/monitored_kdtree_example.cpp)

# Set compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(GTest_FOUND)
        target_compile_options(debug_containers_test PRIVATE -Wall -Wextra -O2)
    endif()
    target_compile_options(memory_debug_example PRIVATE -Wall -Wextra -O2)
    target_compile_options(simple_integration_example PRIVATE -Wall -Wextra -O2)
    target_compile_options(monitored_kdtree_example PRIVATE -Wall -Wextra -O2)
endif()

# Link pthread for memory monitor examples
target_link_libraries(simple_integration_example pthread)
target_link_libraries(monitored_kdtree_example pthread)

# Install rules
install(DIRECTORY include/memory/container_debug/
        DESTINATION include/memory/container_debug/
        FILES_MATCHING PATTERN "*.hpp")

install(FILES include/memory/nanoflann_memory_monitor.hpp
        DESTINATION include/memory/)

install(FILES docs/MEMORY_DEBUG_GUIDE.md docs/ROS_INTEGRATION_GUIDE.md docs/NANOFLANN_MEMORY_MONITOR.md
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
endif()
message(STATUS "    - memory_debug_example")
message(STATUS "    - simple_integration_example")
message(STATUS "    - monitored_kdtree_example")
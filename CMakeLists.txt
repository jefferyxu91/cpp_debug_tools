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

# Find ROS if available (optional)
find_package(rosconsole QUIET)

# Basic tests
add_executable(test_basic test/basic/example.cpp)
add_executable(test_simple test/basic/simple_test.cpp)
add_executable(test_file_line test/basic/file_line_test.cpp)
add_executable(test_function_name test/basic/function_name_test.cpp)
add_executable(test_caller_context test/basic/caller_context_test.cpp)

# Constructor tests
add_executable(test_constructor test/constructor/constructor_test.cpp)

# Comprehensive tests
add_executable(test_comprehensive test/comprehensive/comprehensive_test.cpp)
add_executable(test_final test/comprehensive/final_example.cpp)

# ROS integration tests (only if ROS is found)
if(rosconsole_FOUND)
    add_executable(test_ros_simple test/ros_integration/simple_ros_example.cpp)
    target_link_libraries(test_ros_simple ${catkin_LIBRARIES})
    
    add_executable(test_ros_integration test/ros_integration/ros_integration_example.cpp)
    target_link_libraries(test_ros_integration ${catkin_LIBRARIES})
    
    add_executable(test_ros_real test/ros_integration/real_ros_example.cpp)
    target_link_libraries(test_ros_real ${catkin_LIBRARIES})
    
    message(STATUS "ROS found - ROS integration tests will be built")
else()
    message(STATUS "ROS not found - ROS integration tests will be skipped")
endif()

# Set compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_basic PRIVATE -Wall -Wextra -O2)
    target_compile_options(test_simple PRIVATE -Wall -Wextra -O2)
    target_compile_options(test_file_line PRIVATE -Wall -Wextra -O2)
    target_compile_options(test_function_name PRIVATE -Wall -Wextra -O2)
    target_compile_options(test_caller_context PRIVATE -Wall -Wextra -O2)
    target_compile_options(test_constructor PRIVATE -Wall -Wextra -O2)
    target_compile_options(test_comprehensive PRIVATE -Wall -Wextra -O2)
    target_compile_options(test_final PRIVATE -Wall -Wextra -O2)
    
    if(rosconsole_FOUND)
        target_compile_options(test_ros_simple PRIVATE -Wall -Wextra -O2)
        target_compile_options(test_ros_integration PRIVATE -Wall -Wextra -O2)
        target_compile_options(test_ros_real PRIVATE -Wall -Wextra -O2)
    endif()
endif()

# Install rules
install(DIRECTORY include/memory/container_debug/
        DESTINATION include/memory/container_debug/
        FILES_MATCHING PATTERN "*.hpp")

install(FILES docs/README.md docs/ROS_INTEGRATION_GUIDE.md
        DESTINATION share/debug_containers/docs/)

# Print configuration summary
message(STATUS "Debug Containers Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  ROS Integration: ${rosconsole_FOUND}")
message(STATUS "  Tests to build:")
message(STATUS "    - test_basic")
message(STATUS "    - test_simple")
message(STATUS "    - test_file_line")
message(STATUS "    - test_function_name")
message(STATUS "    - test_caller_context")
message(STATUS "    - test_constructor")
message(STATUS "    - test_comprehensive")
message(STATUS "    - test_final")
if(rosconsole_FOUND)
    message(STATUS "    - test_ros_simple")
    message(STATUS "    - test_ros_integration")
    message(STATUS "    - test_ros_real")
endif()
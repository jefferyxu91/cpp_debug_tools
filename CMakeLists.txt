cmake_minimum_required(VERSION 3.10)
project(nanoflann_memory_monitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Add executable
add_executable(example_memory_monitor example_memory_monitor.cpp)

# Link libraries
target_link_libraries(example_memory_monitor Threads::Threads)

# Set compiler flags
target_compile_options(example_memory_monitor PRIVATE 
    -Wall 
    -Wextra 
    -O2
)

# Include directories
target_include_directories(example_memory_monitor PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Print configuration info
message(STATUS "Building memory-monitored nanoflann example")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
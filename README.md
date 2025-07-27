# Debug Containers

A minimalistic C++ header library for debugging standard library containers with comprehensive memory allocation tracking and ROS integration support.

## Project Structure

```
debug_containers/
├── include/
│   └── memory/
│       └── container_debug/
│           ├── debug_containers.hpp    # Advanced header with context capture
│           └── simple_debug_containers.hpp  # Simple non-intrusive header
├── test/
│   ├── basic/                          # Basic functionality tests
│   │   ├── example.cpp
│   │   └── simple_test.cpp
│   ├── constructor/                    # Constructor allocation tests
│   │   └── constructor_test.cpp
│   ├── comprehensive/                  # Comprehensive tests
│   │   ├── comprehensive_test.cpp
│   │   └── final_example.cpp
│   └── ros_integration/                # ROS integration tests
│       ├── simple_ros_example.cpp
│       ├── ros_integration_example.cpp
│       └── real_ros_example.cpp
├── docs/                               # Documentation
│   ├── README.md                       # Detailed usage guide
│   └── ROS_INTEGRATION_GUIDE.md       # ROS integration guide
├── CMakeLists.txt                      # Build configuration
└── README.md                           # This file
```

## Quick Start

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10 or later
- Optional: ROS development libraries for ROS integration tests

### Building

```bash
# Clone the repository
git clone <repository-url>
cd debug_containers

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make

# Run tests
./test_basic
./test_simple
./test_constructor
./test_comprehensive
./test_final

# If ROS is available, also run ROS tests
./test_ros_simple
./test_ros_integration
```

### Using the Headers

#### Simple Non-Intrusive Approach (Recommended)
```cpp
#include <memory/container_debug/simple_debug_containers.hpp>

int main() {
    // Set memory threshold (default: 20MB)
    Debug::set_memory_threshold(1024 * 1024); // 1MB
    
    // Use debug containers - automatically captures file/line/function
    Debug::vector<int> my_vector(1000); // Will log with caller's location
    
    return 0;
}
```

#### Advanced Context Capture Approach
```cpp
#include <memory/container_debug/debug_containers.hpp>

int main() {
    // Set memory threshold (default: 20MB)
    Debug::set_memory_threshold(1024 * 1024); // 1MB
    
    // Use debug containers with explicit context capture
    Debug::vector<int> my_vector(__FILE__, __LINE__, __FUNCTION__);
    
    // Or use automatic context capture
    AUTO_CONTEXT();
    Debug::vector<int> my_vector2(1000); // Will log with caller's location
    
    return 0;
}
```

## Features

- **Non-Intrusive Design**: Automatically captures caller's file, line, and function without explicit context passing
- **Comprehensive Memory Tracking**: Monitors all allocation types (constructors, copy operations, assignments, reserves)
- **Custom Output Streams**: Redirect warnings to any logging system
- **ROS Integration**: Seamless integration with ROS logging
- **Minimal Overhead**: Zero performance impact when allocations are below threshold
- **Complete Coverage**: All major std containers supported
- **Two Implementation Options**: Simple non-intrusive and advanced context capture approaches

## Available Containers

All standard containers are available with the `Debug::` prefix:

- `Debug::vector<T>`, `Debug::list<T>`, `Debug::deque<T>`
- `Debug::set<T>`, `Debug::multiset<T>`
- `Debug::map<Key, Value>`, `Debug::multimap<Key, Value>`
- `Debug::unordered_set<T>`, `Debug::unordered_multiset<T>`
- `Debug::unordered_map<Key, Value>`, `Debug::unordered_multimap<Key, Value>`
- `Debug::stack<T>`, `Debug::queue<T>`, `Debug::priority_queue<T>`
- `Debug::string`

## Documentation

- [Detailed Usage Guide](docs/README.md) - Complete feature documentation and examples
- [ROS Integration Guide](docs/ROS_INTEGRATION_GUIDE.md) - ROS-specific integration examples

## Examples

The project includes comprehensive examples in the `test/` directory:

- **Basic Tests**: Simple usage examples
- **Constructor Tests**: Demonstrates constructor-based allocation tracking
- **Comprehensive Tests**: Shows all container types and features
- **ROS Integration Tests**: ROS logging integration examples

## Installation

```bash
# Build and install
mkdir build && cd build
cmake ..
make
sudo make install

# The header will be installed to /usr/local/include/memory/container_debug/
# Documentation will be installed to /usr/local/share/debug_containers/docs/
```

## CMake Integration

To use in your own CMake project:

```cmake
# Find the package
find_package(debug_containers REQUIRED)

# Include directories
include_directories(${debug_containers_INCLUDE_DIRS})

# Link libraries (if any)
target_link_libraries(your_target ${debug_containers_LIBRARIES})
```

## License

[Add your license information here]

## Contributing

[Add contribution guidelines here]
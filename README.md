# Debug Containers

A minimalistic C++ header library for debugging standard library containers with comprehensive memory allocation tracking and ROS integration support.

## Project Structure

```
debug_containers/
├── include/
│   └── memory/
│       ├── container_debug/
│       │   └── debug_containers.hpp    # Main header file
│       └── nanoflann_debug/
│           ├── nanoflann_memory_monitor.hpp  # Nanoflann memory monitor
│           └── README.md                      # Nanoflann monitor documentation
├── test/
│   ├── debug_containers_test.cpp       # Google Test suite
│   └── nanoflann_memory_monitor_test.cpp     # Nanoflann monitor tests
├── examples/
│   ├── memory_debug_example.cpp        # Memory debug container example
│   └── example_memory_monitor.cpp      # Nanoflann monitor example
├── docs/                               # Documentation
│   ├── MEMORY_DEBUG_GUIDE.md          # Memory debug usage guide
│   └── ROS_INTEGRATION_GUIDE.md       # ROS integration guide
├── CMakeLists.txt                      # Build configuration
└── README.md                           # This file
```

## Quick Start

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10 or later
- Google Test (GTest) - optional, for running tests
- Optional: ROS development libraries for ROS integration

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

# Run tests (if GTest is available)
./debug_containers_test
./nanoflann_memory_monitor_test

# Run examples
./memory_debug_example
./nanoflann_memory_monitor_example

### Using the Header

```cpp
#include <memory/container_debug/debug_containers.hpp>

int main() {
    // Set memory threshold (default: 20MB)
    Debug::set_memory_threshold(1024 * 1024); // 1MB
    
    // Use debug containers - automatically captures file/line/function
    Debug::vector<int> my_vector(1000); // Will log with caller's location
    
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

- [Memory Debug Guide](docs/MEMORY_DEBUG_GUIDE.md) - Complete memory debug feature documentation and examples
- [ROS Integration Guide](docs/ROS_INTEGRATION_GUIDE.md) - ROS-specific integration examples
- [Nanoflann Memory Monitor](include/memory/nanoflann_debug/README.md) - Memory-monitored nanoflann KD-tree extension

## Examples

The project includes comprehensive examples in the `test/` and `examples/` directories:

### Debug Containers
- **Basic Tests**: Simple usage examples
- **Constructor Tests**: Demonstrates constructor-based allocation tracking
- **Comprehensive Tests**: Shows all container types and features
- **ROS Integration Tests**: ROS logging integration examples

### Nanoflann Memory Monitor
- **Memory Monitor Example**: Demonstrates memory-monitored KD-tree construction
- **Memory Limit Testing**: Shows exception handling when memory limits are exceeded
- **Custom Parameters**: Examples of using custom build parameters
- **Performance Testing**: Comprehensive test suite with 8 test cases

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
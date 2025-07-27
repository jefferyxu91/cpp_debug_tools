# Debug Containers Header

A minimalistic C++ header file that provides debugging capabilities for standard library containers with memory allocation tracking.

## Features

- **Memory Allocation Tracking**: Detects when containers allocate memory larger than a specified threshold
- **File and Line Information**: Prints the exact file and line number where large allocations occur
- **Minimalistic Design**: Simple header-only implementation
- **Namespace Isolation**: All functionality is contained within the `Debug` namespace
- **Complete Coverage**: Supports all major std library containers

## Usage

### Basic Usage

```cpp
#include "debug_containers.hpp"

int main() {
    // Use Debug::vector instead of std::vector
    Debug::vector<int> my_vector;
    
    // Large allocation will trigger debug output
    my_vector.reserve(1000000); // This will print allocation info if > threshold
    
    return 0;
}
```

### Setting Memory Threshold

```cpp
// Set custom memory threshold (default is 20MB)
Debug::set_memory_threshold(1024 * 1024); // 1MB

// Get current threshold
size_t threshold = Debug::get_memory_threshold();
```

### Available Containers

All standard containers are available with the `Debug::` prefix:

- `Debug::vector<T>`
- `Debug::list<T>`
- `Debug::deque<T>`
- `Debug::set<T>`
- `Debug::multiset<T>`
- `Debug::map<Key, Value>`
- `Debug::multimap<Key, Value>`
- `Debug::unordered_set<T>`
- `Debug::unordered_multiset<T>`
- `Debug::unordered_map<Key, Value>`
- `Debug::unordered_multimap<Key, Value>`
- `Debug::stack<T>`
- `Debug::queue<T>`
- `Debug::priority_queue<T>`
- `Debug::string`

## Example Output

When a large allocation is detected, you'll see output like:

```
[DEBUG] Large allocation detected: 2097152 bytes at example.cpp:15
```

## Compilation

Simply include the header in your C++ project:

```bash
g++ -std=c++17 example.cpp -o example
```

## Requirements

- C++17 or later
- Standard library containers

## Notes

- The debug allocator wraps the standard allocator and adds tracking functionality
- All containers maintain the same interface as their std counterparts
- Performance overhead is minimal when allocations are below the threshold
- The threshold can be changed at runtime using `set_memory_threshold()`

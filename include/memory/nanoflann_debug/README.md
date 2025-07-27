# Nanoflann Memory Monitor

A memory-monitored extension for nanoflann that prevents excessive memory usage during KD-tree construction.

## Features

- **Memory Monitoring**: Tracks memory usage during KD-tree construction
- **Configurable Thresholds**: Set custom memory limits
- **Exception Handling**: Throws `MemoryLimitExceededException` when limits are exceeded
- **Minimal Overhead**: Lightweight monitoring with minimal performance impact
- **Seamless Integration**: Drop-in replacement for nanoflann's KD-tree classes

## Quick Start

```cpp
#include "include/memory/nanoflann_debug/nanoflann_memory_monitor.hpp"

// Define your dataset adaptor
struct MyDatasetAdaptor {
    const std::vector<std::array<float, 3>>& points;
    
    inline size_t kdtree_get_point_count() const { return points.size(); }
    inline float kdtree_get_pt(const size_t idx, const size_t dim) const {
        return points[idx][dim];
    }
    template <class BBOX>
    bool kdtree_get_bbox(BBOX& /*bb*/) const { return false; }
};

// Create memory-monitored KD-tree
const size_t memory_threshold = 100 * 1024 * 1024; // 100 MB
MyDatasetAdaptor dataset(points);

try {
    nanoflann::MemoryMonitoredKDTree<
        nanoflann::L2_Simple_Adaptor<float, MyDatasetAdaptor, float, uint32_t>,
        MyDatasetAdaptor,
        3,
        uint32_t> index(3, dataset, memory_threshold);
    
    // Use the index for searches
    std::array<float, 3> query = {0.0f, 0.0f, 0.0f};
    std::vector<uint32_t> indices(5);
    std::vector<float> distances(5);
    size_t num_found = index.knnSearch(query.data(), 5, indices.data(), distances.data());
    
} catch (const nanoflann::MemoryLimitExceededException& e) {
    std::cerr << "Memory limit exceeded: " << e.what() << std::endl;
}
```

## Usage Examples

### Basic Usage
```cpp
// Set memory threshold
const size_t memory_threshold = 50 * 1024 * 1024; // 50 MB

// Create memory-monitored KD-tree
nanoflann::MemoryMonitoredKDTree<...> index(dim, dataset, memory_threshold);
```

### Custom Parameters
```cpp
nanoflann::KDTreeSingleIndexAdaptorParams params;
params.leaf_max_size = 10;
params.n_thread_build = 1;

nanoflann::MemoryMonitoredKDTree<...> index(dim, dataset, params, memory_threshold);
```

### Memory Monitoring
```cpp
// Check current memory usage
size_t current_memory = index.getCurrentMemoryUsage();
size_t threshold = index.getMemoryThreshold();

std::cout << "Memory usage: " << current_memory / 1024 << " KB" << std::endl;
std::cout << "Threshold: " << threshold / 1024 << " KB" << std::endl;
```

## Exception Handling

The memory monitor throws `MemoryLimitExceededException` when the memory threshold is exceeded:

```cpp
try {
    // Build KD-tree with memory monitoring
    nanoflann::MemoryMonitoredKDTree<...> index(dim, dataset, memory_threshold);
} catch (const nanoflann::MemoryLimitExceededException& e) {
    std::cerr << "Memory limit exceeded: " << e.what() << std::endl;
    // Handle the exception (e.g., reduce dataset size, increase threshold)
}
```

## Building and Testing

```bash
# Build the project
mkdir build && cd build
cmake ..
make

# Run the example
./bin/nanoflann_memory_monitor_example

# Run the tests
./bin/nanoflann_memory_monitor_test
```

## When to Use

- Building large KD-trees in memory-constrained environments
- Preventing out-of-memory errors during index construction
- Monitoring memory usage during development and testing
- Ensuring predictable memory behavior in production systems

## Limitations

- Memory monitoring adds minimal overhead (~1-5%)
- Linux systems provide more accurate memory measurements via `/proc/self/status`
- Memory thresholds should be set conservatively to account for system overhead
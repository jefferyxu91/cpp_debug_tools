# Memory-Monitored nanoflann Extension

This project provides a derived class for nanoflann that adds memory monitoring capabilities to interrupt the build index process when memory usage exceeds a specified threshold.

## Features

- **Memory Monitoring**: Real-time memory usage tracking during KD-tree construction
- **Configurable Thresholds**: Set custom memory limits in bytes
- **Minimal Overhead**: Efficient memory checking with minimal performance impact
- **Exception Handling**: Throws `MemoryLimitExceededException` when limits are exceeded
- **Cross-Platform**: Works on Linux and other Unix-like systems
- **Drop-in Replacement**: Compatible with existing nanoflann code

## Requirements

- C++17 or later
- nanoflann v1.6.1
- Linux/Unix system (for accurate memory monitoring)
- CMake 3.10 or later

## Installation

1. Clone the nanoflann repository:
```bash
git clone https://github.com/jlblancoc/nanoflann.git -b v1.6.1
```

2. Copy the memory monitor header to your project:
```bash
cp nanoflann_memory_monitor.hpp /path/to/your/project/
```

3. Include the header in your code:
```cpp
#include "nanoflann_memory_monitor.hpp"
```

## Usage

### Basic Usage

```cpp
#include "nanoflann_memory_monitor.hpp"
#include <vector>

// Define your dataset adaptor
struct MyDatasetAdaptor {
    const std::vector<std::array<float, 3>>& points;
    
    explicit MyDatasetAdaptor(const std::vector<std::array<float, 3>>& pts) 
        : points(pts) {}
    
    inline size_t kdtree_get_point_count() const { return points.size(); }
    inline float kdtree_get_pt(const size_t idx, const size_t dim) const {
        return points[idx][dim];
    }
    template <class BBOX>
    bool kdtree_get_bbox(BBOX& /*bb*/) const { return false; }
};

int main() {
    // Your dataset
    std::vector<std::array<float, 3>> points = /* ... */;
    MyDatasetAdaptor dataset(points);
    
    // Set memory threshold (e.g., 100 MB)
    const size_t memory_threshold = 100 * 1024 * 1024;
    
    try {
        // Create memory-monitored KD-tree
        nanoflann::MemoryMonitoredKDTree<
            nanoflann::metric_L2_Simple, 
            MyDatasetAdaptor, 
            3,  // 3D points
            uint32_t> index(3, dataset, memory_threshold);
        
        // Use the index for searches
        std::array<float, 3> query = {0.0f, 0.0f, 0.0f};
        std::vector<uint32_t> indices(10);
        std::vector<float> distances(10);
        
        size_t num_found = index.knnSearch(query.data(), 10, 
                                         indices.data(), distances.data());
        
    } catch (const nanoflann::MemoryLimitExceededException& e) {
        std::cerr << "Memory limit exceeded: " << e.what() << std::endl;
        // Handle the memory limit exceeded case
    }
    
    return 0;
}
```

### Advanced Usage

```cpp
// Custom parameters
nanoflann::KDTreeSingleIndexAdaptorParams params;
params.leaf_max_size = 20;
params.n_thread_build = 4;

// Create with custom parameters
nanoflann::MemoryMonitoredKDTree<
    nanoflann::metric_L2_Simple, 
    MyDatasetAdaptor, 
    3, 
    uint32_t> index(3, dataset, params, memory_threshold);

// Check memory usage
std::cout << "Current memory: " << index.getCurrentMemoryUsage() / (1024*1024) << " MB\n";
std::cout << "Threshold: " << index.getMemoryThreshold() / (1024*1024) << " MB\n";
```

## Building the Example

```bash
mkdir build
cd build
cmake ..
make
./example_memory_monitor
```

## Memory Monitoring Details

### Memory Measurement Methods

The extension uses different methods to measure memory usage:

1. **Linux**: Uses `/proc/self/status` to read `VmRSS` (Resident Set Size)
2. **Fallback**: Uses `getrusage()` with `RUSAGE_SELF`

### Memory Check Points

Memory is checked at the following points:

1. **Before tree construction**: Initial memory check
2. **During node allocation**: Before each tree node allocation
3. **During tree division**: Before recursive tree building

### Performance Impact

- **Memory checking overhead**: ~1-5 microseconds per check
- **Check frequency**: Once per tree node allocation
- **Total overhead**: Typically <1% of total build time

## Exception Handling

When memory limits are exceeded, the extension throws `MemoryLimitExceededException` with detailed information:

```cpp
try {
    // Build index
    index.buildIndex();
} catch (const nanoflann::MemoryLimitExceededException& e) {
    std::cerr << "Memory limit exceeded: " << e.what() << std::endl;
    // Current memory usage and threshold are included in the message
}
```

## Configuration Options

### Memory Threshold

Set the memory threshold in bytes:

```cpp
const size_t memory_threshold = 100 * 1024 * 1024; // 100 MB
```

### Tree Parameters

Configure tree building parameters:

```cpp
nanoflann::KDTreeSingleIndexAdaptorParams params;
params.leaf_max_size = 10;        // Max points per leaf
params.n_thread_build = 1;        // Number of build threads
params.flags = nanoflann::KDTreeSingleIndexAdaptorFlags::None;
```

## Limitations

1. **Platform Dependency**: Memory monitoring works best on Linux systems
2. **Memory Granularity**: Measurements are approximate (kilobyte granularity)
3. **Thread Safety**: Memory monitoring is not thread-safe in concurrent builds
4. **Memory Type**: Only monitors RSS (Resident Set Size), not virtual memory

## Troubleshooting

### Memory Limit Exceeded Too Early

- Increase the memory threshold
- Reduce dataset size
- Use larger leaf sizes to reduce tree depth

### Inaccurate Memory Measurements

- Ensure running on Linux for best accuracy
- Check if `/proc/self/status` is accessible
- Verify process has sufficient permissions

### Performance Issues

- Memory checking overhead is minimal but can be reduced by increasing check intervals
- Consider using larger memory thresholds to reduce check frequency

## License

This extension follows the same BSD license as nanoflann. See the nanoflann repository for license details.

## Contributing

Contributions are welcome! Please ensure:

1. Code follows the existing style
2. Memory monitoring overhead remains minimal
3. Cross-platform compatibility is maintained
4. Tests are added for new features
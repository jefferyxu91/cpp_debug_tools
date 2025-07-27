# Nanoflann Memory Monitor

A specialized memory monitoring tool designed specifically for nanoflann tree building operations. This tool provides non-intrusive memory tracking with minimal overhead, focusing on detecting memory spikes and threshold violations during tree construction and indexing.

## Quick Start

### Prerequisites

- C++17 compatible compiler
- CMake 3.10 or later
- nanoflann library (included as submodule)

### Building

```bash
# Clone the repository with submodules
git clone --recursive <repository-url>
cd debug_containers

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make

# Run the real nanoflann example
./real_nanoflann_memory_example
```

### Basic Usage

```cpp
#include <memory/nanoflann_memory_monitor.hpp>
#include <nanoflann.hpp>

int main() {
    // Create memory monitor
    auto monitor = nanoflann::memory_utils::create_default_monitor();
    nanoflann::memory_utils::add_standard_logging(monitor);
    monitor.start();
    
    // Your nanoflann tree building code
    {
        nanoflann::TreeBuildScope scope(monitor, "My tree build");
        // ... tree building operations ...
    } // Automatically marks tree build end
    
    monitor.stop();
    return 0;
}
```

## Features

- **Non-intrusive Design**: Minimal performance impact (~1-5% overhead)
- **Tree Building Focus**: Specialized tracking for nanoflann operations
- **RAII Integration**: Automatic scope-based monitoring
- **Cross-Platform**: Works on Linux, Windows, and macOS
- **Event System**: Callback-based event handling
- **Memory Estimation**: Tools to estimate tree construction memory usage
- **Background Monitoring**: Optional continuous monitoring thread

## Examples

### 1. Basic Tree Building Monitoring

```cpp
#include <memory/nanoflann_memory_monitor.hpp>
#include <nanoflann.hpp>

struct PointCloud {
    std::vector<std::vector<double>> pts;
    
    inline size_t kdtree_get_point_count() const { return pts.size(); }
    inline double kdtree_get_pt(const size_t idx, const size_t dim) const {
        return pts[idx][dim];
    }
    template <class BBOX> bool kdtree_get_bbox(BBOX&) const { return false; }
};

void build_tree_with_monitoring() {
    auto monitor = nanoflann::memory_utils::create_default_monitor();
    monitor.start();
    
    PointCloud cloud;
    // ... populate cloud data ...
    
    {
        nanoflann::TreeBuildScope scope(monitor, "Point cloud tree");
        
        using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<double, PointCloud>,
            PointCloud, 3>;
        
        KDTree index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
        index.buildIndex();
    }
    
    monitor.stop();
}
```

### 2. Large-Scale Tree Building

```cpp
void build_large_tree(size_t num_points, size_t dimension) {
    // Estimate memory usage
    size_t estimated_memory = nanoflann::memory_utils::estimate_tree_memory_usage(
        num_points, dimension, sizeof(double));
    
    // Create monitor with appropriate threshold
    auto monitor = nanoflann::memory_utils::create_large_scale_monitor(estimated_memory);
    
    // Add custom callback for large operations
    monitor.add_callback([](const nanoflann::MemoryMonitor::MemoryEvent& event) {
        if (event.type == nanoflann::MemoryMonitor::EventType::THRESHOLD_EXCEEDED) {
            std::cerr << "WARNING: Memory threshold exceeded: " << event.memory_mb << "MB" << std::endl;
        }
    });
    
    monitor.start();
    
    {
        nanoflann::TreeBuildScope scope(monitor, "Large tree build");
        // Build your large tree
    }
    
    monitor.stop();
}
```

### 3. Custom Memory Reporter

```cpp
// Custom memory reporter for specific use cases
size_t custom_memory_reporter() {
    // Your custom memory measurement logic
    return get_custom_memory_usage();
}

void monitor_with_custom_reporter() {
    nanoflann::MemoryMonitor::Config config;
    config.memory_threshold_mb = 100;
    
    nanoflann::MemoryMonitor monitor(config, custom_memory_reporter);
    monitor.start();
    
    // Your code here
    
    monitor.stop();
}
```

## Configuration

### Memory Monitor Configuration

```cpp
nanoflann::MemoryMonitor::Config config;
config.memory_threshold_mb = 100;           // Memory threshold in MB
config.check_interval_ms = 100;             // Memory check interval in milliseconds
config.enable_background_monitoring = true; // Enable background thread monitoring
config.enable_detailed_logging = false;     // Enable detailed memory logging
config.log_prefix = "[NANOFLANN_MEMORY]";   // Log message prefix

nanoflann::MemoryMonitor monitor(config);
```

### Recommended Settings

| Use Case | Threshold | Check Interval | Background Monitoring |
|----------|-----------|----------------|----------------------|
| Development | 50MB | 50ms | Yes |
| Production | 200MB | 200ms | Yes |
| Debugging | 20MB | 10ms | Yes |
| Performance Critical | 500MB | 500ms | No |

## Memory Estimation

The tool provides utilities to estimate memory usage for nanoflann tree construction:

```cpp
size_t estimated_memory = nanoflann::memory_utils::estimate_tree_memory_usage(
    num_points,    // Number of points
    dimension,     // Dimension of points
    sizeof(double) // Size of point type
);
```

This estimation includes:
- Point data storage
- Tree node structures
- Index arrays
- Temporary buffers during construction
- 20% safety overhead

## Event Types

The memory monitor generates various event types:

- `THRESHOLD_EXCEEDED`: Memory threshold exceeded
- `PEAK_MEMORY_REACHED`: New peak memory reached
- `TREE_BUILD_START`: Tree building started
- `TREE_BUILD_END`: Tree building ended
- `MEMORY_SPIKE_DETECTED`: Sudden memory increase detected

## Platform Support

- **Linux**: Uses `/proc/self/status` for memory measurement
- **Windows**: Uses `GetProcessMemoryInfo` API
- **macOS**: Uses Mach kernel APIs
- **Other Platforms**: Falls back to zero memory reporting

## Performance Considerations

### Overhead
- **Background monitoring**: ~1-5% CPU overhead depending on check interval
- **Memory measurement**: ~0.1-1ms per measurement
- **Event processing**: Minimal overhead, depends on callback complexity

### Optimization Tips

1. **Adjust check interval**: Use longer intervals (100-500ms) for production
2. **Disable background monitoring**: For manual-only checking
3. **Limit event history**: Events are automatically limited to 1000 entries
4. **Use custom reporters**: For more efficient memory measurement if needed

## Testing

Run the tests to verify functionality:

```bash
# Run all tests
make test

# Run specific tests
./nanoflann_memory_monitor_test
./real_nanoflann_memory_test
```

## Examples

The project includes several examples:

- `nanoflann_memory_monitor_example`: Basic usage demonstration
- `real_nanoflann_memory_example`: Real nanoflann integration with performance comparison

## Integration with Existing Code

### Minimal Integration

```cpp
// Add to your existing nanoflann code
auto monitor = nanoflann::memory_utils::create_default_monitor();
monitor.start();

// Wrap your tree building code
{
    nanoflann::TreeBuildScope scope(monitor, "Existing tree build");
    // Your existing tree building code here
}

monitor.stop();
```

### Advanced Integration

```cpp
class MonitoredNanoflannIndex {
private:
    nanoflann::MemoryMonitor monitor_;
    // Your nanoflann index
    
public:
    MonitoredNanoflannIndex(const PointCloud& cloud) {
        monitor_.start();
        
        {
            nanoflann::TreeBuildScope scope(monitor_, "Index construction");
            // Build your index
        }
        
        monitor_.stop();
    }
    
    // ... rest of your interface
};
```

## Troubleshooting

### Common Issues

1. **No memory events**: Check if monitoring is active and threshold is appropriate
2. **High overhead**: Increase check interval or disable background monitoring
3. **Inaccurate measurements**: Verify platform support or use custom memory reporter
4. **Memory leaks**: Ensure monitor is properly stopped and callbacks are cleared

### Debug Mode

```cpp
// Enable detailed logging for debugging
nanoflann::MemoryMonitor::Config debug_config;
debug_config.enable_detailed_logging = true;
debug_config.check_interval_ms = 10;  // Very frequent checks

nanoflann::MemoryMonitor debug_monitor(debug_config);
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Submit a pull request

## License

This project is licensed under the same terms as the main debug_containers project.

## Support

For issues and questions:
1. Check the documentation in `docs/NANOFLANN_MEMORY_MONITOR_GUIDE.md`
2. Review the examples in the `examples/` directory
3. Run the tests to verify your setup
4. Open an issue on the project repository
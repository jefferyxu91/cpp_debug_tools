# Nanoflann Memory Monitor Guide

## Overview

The Nanoflann Memory Monitor is a specialized tool designed to track memory usage during nanoflann tree building operations. It provides non-intrusive monitoring with minimal overhead, focusing on detecting memory spikes and threshold violations during tree construction and indexing.

## Key Features

- **Non-intrusive Design**: Minimal performance impact with configurable monitoring intervals
- **Tree Building Focus**: Specialized tracking for nanoflann tree construction operations
- **RAII Integration**: Automatic scope-based monitoring with `TreeBuildScope`
- **Platform Support**: Cross-platform memory usage detection (Windows, Linux, macOS)
- **Event System**: Callback-based event handling for memory events
- **Memory Estimation**: Tools to estimate memory usage for tree construction
- **Background Monitoring**: Optional background thread for continuous monitoring

## Quick Start

### Basic Usage

```cpp
#include <memory/nanoflann_memory_monitor.hpp>

int main() {
    // Create a memory monitor with default settings
    auto monitor = nanoflann::memory_utils::create_default_monitor();
    
    // Add standard logging
    nanoflann::memory_utils::add_standard_logging(monitor);
    
    // Start monitoring
    monitor.start();
    
    // Your nanoflann tree building code here
    {
        nanoflann::TreeBuildScope scope(monitor, "My tree build");
        // ... tree building operations ...
    } // Scope automatically ends here
    
    // Stop monitoring
    monitor.stop();
    
    return 0;
}
```

### Advanced Configuration

```cpp
// Custom configuration
nanoflann::MemoryMonitor::Config config;
config.memory_threshold_mb = 200;        // 200MB threshold
config.check_interval_ms = 25;           // Check every 25ms
config.enable_background_monitoring = true;
config.enable_detailed_logging = true;
config.log_prefix = "[MY_APP_MEMORY]";

nanoflann::MemoryMonitor monitor(config);
```

## API Reference

### MemoryMonitor Class

#### Configuration

```cpp
struct Config {
    size_t memory_threshold_mb = 100;           // Memory threshold in MB
    size_t check_interval_ms = 100;             // Memory check interval in milliseconds
    bool enable_background_monitoring = true;   // Enable background thread monitoring
    bool enable_detailed_logging = false;       // Enable detailed memory logging
    std::string log_prefix = "[NANOFLANN_MEMORY]"; // Log message prefix
};
```

#### Core Methods

- `start()`: Start memory monitoring
- `stop()`: Stop memory monitoring
- `is_active()`: Check if monitoring is active
- `set_threshold(size_t threshold_mb)`: Set memory threshold
- `get_threshold()`: Get current memory threshold
- `check_memory(const std::string& context)`: Manual memory check
- `reset()`: Reset all statistics and event history

#### Tree Building Methods

- `mark_tree_build_start(const std::string& context)`: Mark tree build start
- `mark_tree_build_end(const std::string& context)`: Mark tree build end

#### Statistics and Events

- `get_stats()`: Get current memory statistics
- `get_event_history()`: Get memory event history
- `add_callback(MemoryCallback callback)`: Add event callback
- `clear_callbacks()`: Clear all callbacks

### TreeBuildScope Class

RAII wrapper for automatic tree build tracking:

```cpp
class TreeBuildScope {
public:
    TreeBuildScope(MemoryMonitor& monitor, const std::string& context = "");
    ~TreeBuildScope();
    void end(const std::string& context = "");
};
```

### Memory Event Types

```cpp
enum class EventType {
    THRESHOLD_EXCEEDED,      // Memory threshold exceeded
    PEAK_MEMORY_REACHED,     // New peak memory reached
    TREE_BUILD_START,        // Tree building started
    TREE_BUILD_END,          // Tree building ended
    MEMORY_SPIKE_DETECTED    // Sudden memory increase detected
};
```

## Usage Patterns

### 1. Basic Tree Building Monitoring

```cpp
#include <memory/nanoflann_memory_monitor.hpp>

void build_nanoflann_tree() {
    auto monitor = nanoflann::memory_utils::create_default_monitor();
    nanoflann::memory_utils::add_standard_logging(monitor);
    monitor.start();
    
    {
        nanoflann::TreeBuildScope scope(monitor, "Point cloud tree");
        
        // Your nanoflann tree building code
        // nanoflann::KDTreeSingleIndexAdaptor<...> index(...);
        // index.buildIndex();
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
            std::cerr << "WARNING: Large tree building exceeded memory threshold!" << std::endl;
            // Implement your handling logic here
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

### 4. Event-Driven Monitoring

```cpp
void event_driven_monitoring() {
    auto monitor = nanoflann::memory_utils::create_default_monitor();
    
    // Add multiple callbacks for different purposes
    monitor.add_callback([](const nanoflann::MemoryMonitor::MemoryEvent& event) {
        // Log to file
        log_to_file(event);
    });
    
    monitor.add_callback([](const nanoflann::MemoryMonitor::MemoryEvent& event) {
        // Send alert if critical
        if (event.type == nanoflann::MemoryMonitor::EventType::THRESHOLD_EXCEEDED) {
            send_alert("Memory threshold exceeded: " + std::to_string(event.memory_mb) + "MB");
        }
    });
    
    monitor.add_callback([](const nanoflann::MemoryMonitor::MemoryEvent& event) {
        // Update UI
        update_memory_display(event.memory_mb);
    });
    
    monitor.start();
    
    // Your code here
    
    monitor.stop();
}
```

## Memory Estimation

The tool provides utilities to estimate memory usage for nanoflann tree construction:

```cpp
size_t estimate_tree_memory_usage(size_t num_points, 
                                 size_t dimension, 
                                 size_t point_type_size = sizeof(double));
```

This estimation includes:
- Point data storage
- Tree node structures
- Index arrays
- Temporary buffers during construction
- 20% safety overhead

### Example Estimation

```cpp
// Estimate memory for 1M points in 3D
size_t estimated_mb = nanoflann::memory_utils::estimate_tree_memory_usage(
    1000000,  // 1M points
    3,        // 3D
    sizeof(double)  // double precision
);

std::cout << "Estimated memory usage: " << estimated_mb << "MB" << std::endl;
```

## Platform Support

The memory monitor supports multiple platforms:

### Linux
- Uses `/proc/self/status` for memory measurement
- Reports RSS (Resident Set Size) memory usage

### Windows
- Uses `GetProcessMemoryInfo` API
- Reports Working Set Size

### macOS
- Uses Mach kernel APIs
- Reports resident memory size

### Other Platforms
- Falls back to zero memory reporting
- Custom memory reporters can be provided

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

### Recommended Settings

```cpp
// Development/Debugging
nanoflann::MemoryMonitor::Config debug_config;
debug_config.memory_threshold_mb = 50;
debug_config.check_interval_ms = 50;
debug_config.enable_detailed_logging = true;

// Production
nanoflann::MemoryMonitor::Config prod_config;
prod_config.memory_threshold_mb = 200;
prod_config.check_interval_ms = 200;
debug_config.enable_detailed_logging = false;
```

## Integration with Nanoflann

### Typical Integration Pattern

```cpp
#include <nanoflann.hpp>
#include <memory/nanoflann_memory_monitor.hpp>

template<typename PointCloud>
class MonitoredNanoflannIndex {
private:
    nanoflann::MemoryMonitor monitor_;
    nanoflann::KDTreeSingleIndexAdaptor<...> index_;
    
public:
    MonitoredNanoflannIndex(const PointCloud& cloud) {
        monitor_.start();
        
        {
            nanoflann::TreeBuildScope scope(monitor_, "Index construction");
            index_.buildIndex();
        }
        
        monitor_.stop();
    }
    
    // ... rest of your index interface
};
```

### Memory Threshold Guidelines

| Data Size | Recommended Threshold | Check Interval |
|-----------|----------------------|----------------|
| < 100K points | 50MB | 100ms |
| 100K - 1M points | 200MB | 50ms |
| 1M - 10M points | 1GB | 25ms |
| > 10M points | 2GB+ | 10ms |

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

## Best Practices

1. **Use RAII**: Always use `TreeBuildScope` for automatic cleanup
2. **Set appropriate thresholds**: Base thresholds on expected data size
3. **Handle events**: Implement proper event handling for production use
4. **Monitor in development**: Use detailed logging during development
5. **Estimate memory**: Use estimation tools to set appropriate thresholds
6. **Clean up**: Always stop monitoring and clear callbacks when done

## Example Applications

See `examples/nanoflann_memory_monitor_example.cpp` for a complete working example demonstrating all features of the memory monitor.
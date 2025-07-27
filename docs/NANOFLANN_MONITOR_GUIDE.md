# Nanoflann Memory Monitor Guide

## Overview

The Nanoflann Memory Monitor is a lightweight, non-intrusive tool designed to monitor memory usage during nanoflann KD-tree construction operations. It runs in a background thread and reports when memory usage exceeds specified thresholds.

## Features

- **Real-time monitoring**: Tracks RSS (Resident Set Size) and optionally VSS (Virtual Set Size)
- **Cross-platform**: Works on Linux, Windows, and macOS
- **Low overhead**: Configurable check intervals to minimize performance impact
- **Flexible reporting**: Custom logging functions and threshold configuration
- **Multiple usage patterns**: Manual control, RAII scoped monitoring, and one-shot measurements

## Quick Start

```cpp
#include <memory/nanoflann_monitor.hpp>
#include "nanoflann.hpp"

// Simple scoped monitoring
{
    NanoflannMonitor::ScopedMemoryMonitor monitor("KD-tree construction");
    
    // Your nanoflann tree building code here
    my_kd_tree_t index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
    index.buildIndex();
}
// Monitor automatically stops and reports when leaving scope
```

## Configuration

```cpp
NanoflannMonitor::MonitorConfig config;
config.threshold_mb = 100;          // Alert when memory exceeds 100MB
config.check_interval_ms = 50;      // Check every 50ms
config.monitor_rss = true;          // Monitor Resident Set Size
config.monitor_vss = false;         // Don't monitor Virtual Set Size
config.print_to_stderr = true;      // Print to stderr
```

## Usage Patterns

### 1. Manual Monitoring

```cpp
NanoflannMonitor::MemoryMonitor monitor(config);
monitor.start();

// Perform KD-tree operations
build_kdtree();

monitor.stop();
std::cout << "Peak memory: " << monitor.get_peak_stats().rss_bytes / (1024.0 * 1024.0) << " MB\n";
```

### 2. RAII Scoped Monitoring

```cpp
{
    NanoflannMonitor::ScopedMemoryMonitor monitor("My operation", config);
    // Memory monitoring is active within this scope
    build_kdtree();
}
// Automatically stops and reports when leaving scope
```

### 3. One-shot Measurement

```cpp
NanoflannMonitor::measure_memory_usage(
    []() {
        build_kdtree();
    },
    "KD-tree construction"
);
```

### 4. Custom Logging

```cpp
config.custom_logger = [](const std::string& message) {
    // Write to file, send to monitoring service, etc.
    my_logger.log(message);
};
config.print_to_stderr = false;  // Disable default output
```

## Example Output

```
[NANOFLANN MONITOR] Started monitoring
  Baseline RSS: 118.61 MB
  Threshold: 100 MB
  Check interval: 50 ms
[NANOFLANN MONITOR] Memory threshold exceeded!
  Current RSS: 150.23 MB
  Threshold: 100 MB
  Exceeded by: 50.23 MB
  Times exceeded: 1
[NANOFLANN MONITOR] Stopped monitoring
  Final RSS: 158.09 MB
  Peak RSS: 202.36 MB
  Memory growth: 39.49 MB
  Threshold exceeded: 71 times
```

## Performance Considerations

- The monitor runs in a separate thread to avoid blocking KD-tree construction
- Default check interval is 100ms, which provides good monitoring without significant overhead
- For very fast operations, consider using one-shot measurement instead of continuous monitoring
- The monitor uses platform-specific APIs for minimal overhead:
  - Linux: Reads from `/proc/self/status`
  - Windows: Uses `GetProcessMemoryInfo`
  - macOS: Uses `task_info`

## Integration with Nanoflann

The monitor is designed to be completely non-intrusive. Simply wrap your nanoflann operations:

```cpp
// Before
my_kd_tree_t index(3, cloud, params);
index.buildIndex();

// After (with monitoring)
{
    NanoflannMonitor::ScopedMemoryMonitor monitor("Building KD-tree");
    my_kd_tree_t index(3, cloud, params);
    index.buildIndex();
}
```

## Best Practices

1. **Set appropriate thresholds**: Base your threshold on expected memory usage for your data size
2. **Use scoped monitoring**: Prefer RAII pattern for automatic cleanup
3. **Adjust check intervals**: For long operations, increase interval to reduce overhead
4. **Monitor specific operations**: Focus on tree building (`buildIndex()`) where most allocation occurs
5. **Log to file for production**: Use custom logger to avoid console spam in production

## Troubleshooting

- **No output**: Check that threshold is set appropriately (not too high)
- **Too much output**: Increase check interval or raise threshold
- **Platform not supported**: The tool will compile but return zero memory stats
- **Memory growth calculation**: Negative values may indicate memory was freed during monitoring
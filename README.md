# Debug Containers & NanoFlann Memory Monitor

This repository provides advanced debugging containers and memory monitoring tools for C++ applications, with specialized support for the nanoflann library.

## 🔥 New: NanoFlann Memory Monitor

A powerful, low-overhead memory monitoring tool specifically designed for nanoflann KD-tree operations. Monitor memory usage during tree construction and detect when thresholds are exceeded - automatically!

### ✨ Key Features

- **🔄 Drop-in Replacement**: `MonitoredKDTreeSingleIndexAdaptor` works exactly like nanoflann's `KDTreeSingleIndexAdaptor`
- **📊 Automatic Monitoring**: Memory tracking starts automatically during tree construction
- **⚠️ Smart Alerts**: Configurable threshold-based alerts with custom callbacks
- **⚡ Zero Overhead**: Minimal performance impact (< 1% overhead)
- **🎯 Non-Intrusive**: No changes needed to existing nanoflann code
- **📈 Process-Based**: Tracks actual system memory usage, not just allocations

### 🚀 Quick Start

Replace your nanoflann KDTree with the monitored version:

```cpp
#include <memory/nanoflann_memory_monitor.hpp>

// Before: Regular nanoflann
nanoflann::KDTreeSingleIndexAdaptor<
    nanoflann::L2_Simple_Adaptor<float, PointCloud>,
    PointCloud, 3
> index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));

// After: Monitored nanoflann (automatic memory tracking!)
NanoFlannMemory::MonitoredKDTree3D<float, PointCloud> index(
    3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10)
);
```

**That's it!** The monitored version automatically:
- 🔍 Prints memory usage during construction
- ⚠️ Alerts when memory exceeds thresholds (default: 50MB)
- 📊 Tracks peak memory usage
- ✅ Works with all existing nanoflann functionality

### 📋 Example Output

```
🔍 [NanoFlann Monitor] Starting KD-tree construction with 100000 points...
   Memory threshold: 50.00 MB
✅ [NanoFlann Monitor] KD-tree construction completed!
   Memory used for construction: 4.52 MB
   Total process memory: 12.18 MB
```

If memory exceeds the threshold:
```
⚠️  [NanoFlann Monitor] MEMORY THRESHOLD EXCEEDED!
=== NanoFlann Memory Monitor Report ===
Memory Usage Since Monitor Start: 67.3 MB
Peak Usage During Monitoring: 67.3 MB
Threshold: 50.0 MB
Threshold Exceeded: Yes
=======================================
```

### 🛠️ Advanced Configuration

```cpp
// Custom monitoring configuration
NanoFlannMemory::MonitorConfig config;
config.threshold_bytes = 100 * 1024 * 1024;  // 100MB threshold
config.auto_print_on_exceed = true;          // Auto-print alerts
config.print_construction_summary = false;   // Quiet mode

// Custom alert callback
config.threshold_callback = [](size_t usage, const std::string& msg) {
    std::cout << "🚨 High memory usage: " << (usage/(1024*1024)) << " MB!" << std::endl;
    // Take action: log, reduce dataset size, etc.
};

// Create monitored tree with custom config
NanoFlannMemory::MonitoredKDTree3D<float, PointCloud> index(
    3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10), config
);
```

## 📦 Original Debug Containers

Provides debug-enabled versions of standard C++ containers that automatically detect large memory allocations:

### Features

- **Memory Threshold Detection**: Automatically logs when container allocations exceed configurable thresholds
- **Zero Code Changes**: Drop-in replacements for standard containers
- **Multiple Container Types**: Support for vector, map, set, unordered_map, unordered_set, list, deque, etc.
- **Custom Output Streams**: Redirect debug output to custom streams or files
- **Flexible Configuration**: Configurable memory thresholds and output formatting

### Basic Usage

```cpp
#include <memory/container_debug/debug_containers.hpp>

// Set threshold (default: 20MB)
Debug::set_memory_threshold(1024 * 1024); // 1MB

// Use debug containers - they work exactly like std containers
Debug::vector<int> large_vector;
large_vector.reserve(500000); // This will trigger debug output

Debug::map<int, std::string> debug_map;
Debug::unordered_set<int> debug_set;
```

## 🏗️ Building

```bash
mkdir build && cd build
cmake ..
make
```

### Build Options

- **C++17 Support**: Modern C++ features for better performance
- **Optional GTest**: Unit tests (install with `sudo apt-get install libgtest-dev`)
- **Cross-Platform**: Works on Linux, macOS, and Windows

## 📚 Examples

The repository includes several comprehensive examples:

- `simple_integration_example`: Basic memory monitoring integration
- `monitored_kdtree_example`: Advanced MonitoredKDTree usage with all features
- `memory_debug_example`: Standard container debugging
- `real_nanoflann_memory_example`: Performance comparisons and real-world usage

## 🔧 Requirements

- **C++17** or later
- **CMake** 3.10+
- **nanoflann** (included as submodule)
- **pthread** (for background monitoring)

## 📖 Documentation

- [NanoFlann Memory Monitor Guide](docs/NANOFLANN_MEMORY_MONITOR.md) - Complete guide for memory monitoring
- [Memory Debug Guide](docs/MEMORY_DEBUG_GUIDE.md) - Container debugging documentation

## 🎯 Use Cases

### For NanoFlann Users
- **Performance Tuning**: Understand memory usage patterns during tree construction
- **Resource Planning**: Set appropriate memory limits for different dataset sizes  
- **Debugging**: Detect memory leaks or unexpected allocations
- **Production Monitoring**: Alert on high memory usage in deployed applications

### For C++ Developers
- **Container Debugging**: Track large allocations in standard containers
- **Memory Profiling**: Lightweight alternative to heavy profiling tools
- **Development Debugging**: Catch memory issues during development

## 🤝 Contributing

Contributions welcome! Please see our contribution guidelines and submit pull requests for any improvements.

## 📄 License

Licensed under the BSD License. See `COPYING` for details.

---

⭐ **Star this repo** if you find it useful for your nanoflann projects!
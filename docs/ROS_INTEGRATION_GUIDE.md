# ROS Integration Guide for Debug Containers

This guide shows how to integrate the debug containers header with ROS logging systems to automatically redirect memory allocation warnings to ROS logging streams.

## Overview

The debug containers header now supports custom output streams, allowing you to redirect memory allocation warnings to:
- ROS_WARN_STREAM
- ROS_ERROR_STREAM  
- ROS_INFO_STREAM
- ROS_DEBUG_STREAM
- Custom logging systems
- File streams
- Multiple outputs simultaneously

## Basic ROS Integration

### 1. Include Required Headers

```cpp
#include <memory/container_debug/debug_containers.hpp>
#include <ros/console.h>
```

### 2. Set Up ROS Logging

```cpp
// Basic ROS_WARN_STREAM integration
Debug::set_output_stream([](const std::string& message) {
    ROS_WARN_STREAM("[MEMORY_DEBUG] " << message);
});

// Use debug containers
Debug::vector<int> my_vector(1000); // Will log to ROS_WARN_STREAM
```

## Advanced ROS Integration Examples

### Example 1: Different ROS Log Levels Based on Allocation Size

```cpp
Debug::set_output_stream([](const std::string& message) {
    // Parse allocation size from message
    size_t pos = message.find("detected: ");
    if (pos != std::string::npos) {
        size_t end_pos = message.find(" bytes", pos);
        if (end_pos != std::string::npos) {
            std::string size_str = message.substr(pos + 10, end_pos - pos - 10);
            int size = std::stoi(size_str);
            
            if (size > 10000000) {        // > 10MB
                ROS_ERROR_STREAM("[CRITICAL_MEMORY] " << message);
            } else if (size > 1000000) {  // > 1MB
                ROS_WARN_STREAM("[LARGE_MEMORY] " << message);
            } else if (size > 100000) {   // > 100KB
                ROS_INFO_STREAM("[MEDIUM_MEMORY] " << message);
            } else {
                ROS_DEBUG_STREAM("[SMALL_MEMORY] " << message);
            }
        }
    }
});
```

### Example 2: ROS Logging with Node Context

```cpp
Debug::set_output_stream([](const std::string& message) {
    // Extract file and line information
    size_t file_pos = message.find(" at ");
    if (file_pos != std::string::npos) {
        std::string file_info = message.substr(file_pos + 4);
        std::string allocation_info = message.substr(0, file_pos);
        
        ROS_WARN_STREAM("[NODE_MEMORY_DEBUG] " 
                       << allocation_info 
                       << " in file: " << file_info);
    } else {
        ROS_WARN_STREAM("[NODE_MEMORY_DEBUG] " << message);
    }
});
```

### Example 3: ROS Logging with Timestamp and Performance Context

```cpp
#include <ros/console.h>
#include <ctime>

Debug::set_output_stream([](const std::string& message) {
    time_t now = time(nullptr);
    std::string timestamp = std::to_string(now);
    
    ROS_WARN_STREAM("[PERF_MEMORY][" << timestamp << "] " 
                   << message 
                   << " - Consider optimization");
});
```

### Example 4: Multiple ROS Outputs

```cpp
Debug::set_output_stream([](const std::string& message) {
    // Log to multiple ROS levels
    ROS_WARN_STREAM("[MEMORY_WARN] " << message);
    ROS_INFO_STREAM("[MEMORY_INFO] " << message);
    ROS_DEBUG_STREAM("[MEMORY_DEBUG] " << message);
});
```

### Example 5: ROS Logging with Custom Formatting

```cpp
Debug::set_output_stream([](const std::string& message) {
    // Custom formatting for ROS logs
    std::string formatted_message = "[MEMORY_ALLOCATION] " + message;
    formatted_message += " - Check for memory leaks!";
    
    ROS_WARN_STREAM(formatted_message);
});
```

## Real ROS Node Example

Here's a complete example of a ROS node using debug containers:

```cpp
#include <ros/ros.h>
#include <memory/container_debug/debug_containers.hpp>
#include <vector>
#include <map>

class MemoryDebugNode {
private:
    ros::NodeHandle nh_;
    ros::Timer timer_;
    
    // Use debug containers instead of std containers
    Debug::vector<int> data_vector_;
    Debug::map<int, std::string> data_map_;
    Debug::unordered_map<int, double> cache_;
    
public:
    MemoryDebugNode() {
        // Set up ROS logging for memory allocations
        setupMemoryLogging();
        
        // Set memory threshold (e.g., 1MB)
        Debug::set_memory_threshold(1024 * 1024);
        
        // Set up timer for periodic operations
        timer_ = nh_.createTimer(ros::Duration(1.0), 
                               &MemoryDebugNode::timerCallback, this);
    }
    
private:
    void setupMemoryLogging() {
        Debug::set_output_stream([](const std::string& message) {
            // Parse allocation size
            size_t pos = message.find("detected: ");
            if (pos != std::string::npos) {
                size_t end_pos = message.find(" bytes", pos);
                if (end_pos != std::string::npos) {
                    std::string size_str = message.substr(pos + 10, end_pos - pos - 10);
                    int size = std::stoi(size_str);
                    
                    if (size > 5000000) {        // > 5MB
                        ROS_ERROR_STREAM("[CRITICAL] " << message);
                    } else if (size > 1000000) {  // > 1MB
                        ROS_WARN_STREAM("[WARNING] " << message);
                    } else {
                        ROS_INFO_STREAM("[INFO] " << message);
                    }
                }
            }
        });
    }
    
    void timerCallback(const ros::TimerEvent& event) {
        // Simulate large memory allocations
        if (data_vector_.size() < 100000) {
            data_vector_.reserve(data_vector_.size() + 10000);
            ROS_INFO("Reserved additional space for vector");
        }
        
        // Simulate map operations
        for (int i = 0; i < 1000; ++i) {
            data_map_[i] = "value_" + std::to_string(i);
        }
        
        // Simulate cache operations
        for (int i = 0; i < 500; ++i) {
            cache_[i] = i * 3.14;
        }
    }
};

int main(int argc, char** argv) {
    ros::init(argc, argv, "memory_debug_node");
    
    MemoryDebugNode node;
    
    ROS_INFO("Memory debug node started");
    ros::spin();
    
    return 0;
}
```

## File Logging Integration

You can also log to files while using ROS:

```cpp
#include <fstream>

// Set up file logging
std::ofstream memory_log("memory_allocations.log");

Debug::set_output_stream([](const std::string& message) {
    // Log to ROS
    ROS_WARN_STREAM("[MEMORY] " << message);
    
    // Also log to file
    std::ofstream file("memory_allocations.log", std::ios::app);
    file << "[" << time(nullptr) << "] " << message << std::endl;
    file.close();
});
```

## Conditional Logging

You can enable/disable logging based on ROS parameters:

```cpp
bool enable_memory_logging;
nh_.param("enable_memory_logging", enable_memory_logging, true);

if (enable_memory_logging) {
    Debug::set_output_stream([](const std::string& message) {
        ROS_WARN_STREAM("[MEMORY_DEBUG] " << message);
    });
} else {
    // Disable logging by setting to empty function
    Debug::set_output_stream([](const std::string& message) {});
}
```

## Performance Considerations

- The debug containers add minimal overhead when allocations are below the threshold
- ROS logging can be expensive, so consider using appropriate log levels
- For high-frequency allocations, consider using ROS_DEBUG_STREAM instead of ROS_WARN_STREAM
- You can dynamically adjust the memory threshold based on runtime conditions

## Best Practices

1. **Set Appropriate Thresholds**: Choose memory thresholds that make sense for your application
2. **Use Appropriate Log Levels**: Use ROS_ERROR for critical issues, ROS_WARN for warnings, etc.
3. **Include Context**: Add relevant context to your log messages
4. **Performance Monitoring**: Use timestamps and performance metrics in your logs
5. **File Logging**: Consider logging to files for post-analysis
6. **Conditional Logging**: Enable/disable logging based on debug flags or ROS parameters

## Troubleshooting

### Common Issues

1. **No Output**: Check if the memory threshold is set correctly
2. **Too Much Output**: Increase the memory threshold or use more restrictive logging
3. **Performance Issues**: Use ROS_DEBUG_STREAM for frequent allocations
4. **Compilation Errors**: Ensure you're using C++17 or later

### Debug Tips

- Start with a low threshold (e.g., 1KB) to see all allocations
- Gradually increase the threshold to focus on larger allocations
- Use different ROS log levels to categorize allocations by size
- Monitor your application's memory usage patterns

## Summary

The debug containers header provides flexible integration with ROS logging systems, allowing you to:

- Automatically redirect memory allocation warnings to ROS streams
- Use different log levels based on allocation size
- Add context and timestamps to memory allocation logs
- Log to multiple outputs simultaneously
- Integrate with existing ROS node infrastructure

This integration makes it easy to monitor memory usage in ROS applications and identify potential memory issues during development and debugging.
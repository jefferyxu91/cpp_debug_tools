/**
 * Nanoflann Memory Monitor Example
 * 
 * This example demonstrates how to monitor memory usage during nanoflann
 * KD-tree construction operations. The monitor is designed to be non-intrusive
 * and low-overhead, running in a background thread.
 * 
 * To use this example, you'll need to have nanoflann installed.
 * If not installed, you can get it from: https://github.com/jlblancoc/nanoflann
 */

#include <memory/nanoflann_monitor.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>

#include "nanoflann.hpp"

// For demonstration purposes, we'll simulate a KD-tree build operation
void simulate_kdtree_build(size_t num_points, size_t dimensions) {
    std::cout << "\nSimulating KD-tree build with " << num_points 
              << " points in " << dimensions << "D space..." << std::endl;
    
    // Allocate memory for points (simulating what nanoflann would do)
    std::vector<std::vector<double>> points(num_points);
    
    // Generate random points
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-100.0, 100.0);
    
    for (size_t i = 0; i < num_points; ++i) {
        points[i].resize(dimensions);
        for (size_t j = 0; j < dimensions; ++j) {
            points[i][j] = dis(gen);
        }
    }
    
    // Simulate tree construction delay
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

// Example 1: Using manual start/stop monitoring
void example_manual_monitoring() {
    std::cout << "\n=== Example 1: Manual Monitoring ===" << std::endl;
    
    // Create monitor with custom configuration
    NanoflannMonitor::MonitorConfig config;
    config.threshold_mb = 50;           // Alert when memory exceeds 50MB
    config.check_interval_ms = 50;      // Check every 50ms
    config.monitor_rss = true;          // Monitor resident set size
    config.monitor_vss = false;         // Don't monitor virtual size
    
    NanoflannMonitor::MemoryMonitor monitor(config);
    
    // Start monitoring
    monitor.start();
    
    // Perform operations that allocate memory
    simulate_kdtree_build(1000000, 3);    // 1M points in 3D
    simulate_kdtree_build(2000000, 3);    // 2M points in 3D
    
    // Stop monitoring
    monitor.stop();
    
    // Get statistics
    std::cout << "\nThreshold exceeded " << monitor.get_threshold_exceeded_count() 
              << " times during monitoring." << std::endl;
}

// Example 2: Using RAII scoped monitoring
void example_scoped_monitoring() {
    std::cout << "\n=== Example 2: Scoped Monitoring ===" << std::endl;
    
    NanoflannMonitor::MonitorConfig config;
    config.threshold_mb = 30;
    config.check_interval_ms = 25;  // More frequent checks
    
    {
        // Monitor automatically starts when entering scope
        NanoflannMonitor::ScopedMemoryMonitor monitor("Large KD-tree construction", config);
        
        simulate_kdtree_build(500000, 10);   // 500k points in 10D
        simulate_kdtree_build(1000000, 10);  // 1M points in 10D
        
        // Monitor automatically stops when leaving scope
    }
}

// Example 3: Using one-shot memory measurement
void example_oneshot_measurement() {
    std::cout << "\n=== Example 3: One-shot Measurement ===" << std::endl;
    
    // Measure memory usage of a specific operation
    NanoflannMonitor::measure_memory_usage(
        []() {
            simulate_kdtree_build(3000000, 5);  // 3M points in 5D
        },
        "3M point KD-tree construction"
    );
}

// Example 4: Using custom logger
void example_custom_logger() {
    std::cout << "\n=== Example 4: Custom Logger ===" << std::endl;
    
    // Create a custom logger that writes to a file or custom output
    auto custom_logger = [](const std::string& message) {
        // In real usage, this could write to a log file, send to monitoring service, etc.
        std::cout << "[CUSTOM LOG] " << message << std::endl;
    };
    
    NanoflannMonitor::MonitorConfig config;
    config.threshold_mb = 40;
    config.custom_logger = custom_logger;
    config.print_to_stderr = false;  // Disable default stderr output
    
    NanoflannMonitor::MemoryMonitor monitor(config);
    monitor.start();
    
    simulate_kdtree_build(1500000, 4);  // 1.5M points in 4D
    
    monitor.stop();
}

// Real nanoflann example
template <typename T>
struct PointCloud {
    std::vector<T> pts;
    
    inline size_t kdtree_get_point_count() const { return pts.size() / 3; }
    
    inline T kdtree_get_pt(const size_t idx, const size_t dim) const {
        return pts[idx * 3 + dim];
    }
    
    template <class BBOX>
    bool kdtree_get_bbox(BBOX&) const { return false; }
};

void example_real_nanoflann() {
    std::cout << "\n=== Real Nanoflann Example ===" << std::endl;
    
    // Configure monitoring
    NanoflannMonitor::MonitorConfig config;
    config.threshold_mb = 100;
    config.check_interval_ms = 50;
    
    // Create point cloud
    PointCloud<double> cloud;
    const size_t num_points = 5000000;  // 5M points
    
    // Generate random points
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-1000.0, 1000.0);
    
    cloud.pts.resize(num_points * 3);
    for (size_t i = 0; i < num_points * 3; ++i) {
        cloud.pts[i] = dis(gen);
    }
    
    // Monitor KD-tree construction
    {
        NanoflannMonitor::ScopedMemoryMonitor monitor("Nanoflann KD-tree build", config);
        
        // Build KD-tree
        using my_kd_tree_t = nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<double, PointCloud<double>>,
            PointCloud<double>,
            3
        >;
        
        my_kd_tree_t index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
        index.buildIndex();
        
        std::cout << "KD-tree built with " << num_points << " points" << std::endl;
    }
}

int main() {
    std::cout << "Nanoflann Memory Monitor Examples" << std::endl;
    std::cout << "=================================" << std::endl;
    
    // Run examples
    example_manual_monitoring();
    example_scoped_monitoring();
    example_oneshot_measurement();
    example_custom_logger();
    
    example_real_nanoflann();
    
    std::cout << "\nAll examples completed!" << std::endl;
    
    return 0;
}
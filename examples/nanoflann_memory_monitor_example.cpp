/**
 * NanoFlann Memory Monitor Example
 * 
 * This example demonstrates how to use the memory monitoring tool
 * to track memory usage during nanoflann tree building and operations.
 * 
 * Features demonstrated:
 * - Setting up memory monitoring with custom thresholds
 * - Using the TrackedAllocator with nanoflann
 * - Monitoring memory during tree construction
 * - Getting memory reports and statistics
 * - Custom callback functions for threshold alerts
 */

#include <memory/nanoflann_memory_monitor.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>

// Simulate nanoflann-style data structures for demonstration
template<typename T, typename Allocator = std::allocator<T>>
class MockKDTreeIndex {
private:
    std::vector<T, Allocator> data_;
    std::vector<int, typename Allocator::template rebind<int>::other> indices_;
    
public:
    template<typename PointCloud>
    MockKDTreeIndex(const PointCloud& cloud, size_t max_leaf_size = 10) {
        // Simulate tree building with allocations
        size_t n_points = cloud.size();
        
        // Allocate space for tree nodes (simulated)
        data_.reserve(n_points * 2);  // Tree typically needs more space
        indices_.reserve(n_points * 3);  // Index arrays
        
        // Simulate tree construction
        for (size_t i = 0; i < n_points; ++i) {
            data_.push_back(cloud[i]);
            indices_.push_back(static_cast<int>(i));
            
            // Simulate internal tree structure allocations
            if (i % 1000 == 0) {
                // Simulate periodic large allocations during tree building
                std::vector<T, Allocator> temp_data(1000);
                data_.insert(data_.end(), temp_data.begin(), temp_data.end());
            }
        }
        
        std::cout << "Tree built with " << n_points << " points" << std::endl;
    }
    
    size_t size() const { return data_.size(); }
};

// Simple point cloud for testing
struct Point3D {
    double x, y, z;
    Point3D(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
};

class PointCloud {
private:
    std::vector<Point3D> points_;
    
public:
    PointCloud(size_t n_points) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dis(-100.0, 100.0);
        
        points_.reserve(n_points);
        for (size_t i = 0; i < n_points; ++i) {
            points_.emplace_back(dis(gen), dis(gen), dis(gen));
        }
    }
    
    size_t size() const { return points_.size(); }
    const Point3D& operator[](size_t i) const { return points_[i]; }
};

// Custom callback functions
void threshold_alert_callback(size_t current_usage, const std::string& message) {
    std::cout << "CUSTOM ALERT: " << message << std::endl;
    std::cout << "Consider reducing point cloud size or increasing available memory!" << std::endl;
}

void periodic_report_callback(size_t current, size_t peak, size_t alloc_count) {
    static int report_counter = 0;
    if (++report_counter % 10 == 0) {  // Report every 10 cycles
        std::cout << "[PERIODIC] Memory: " << (current / (1024.0 * 1024.0)) 
                  << " MB current, " << (peak / (1024.0 * 1024.0)) 
                  << " MB peak, " << alloc_count << " allocations" << std::endl;
    }
}

int main() {
    std::cout << "=== NanoFlann Memory Monitor Example ===" << std::endl;
    
    // Example 1: Basic monitoring with default settings
    std::cout << "\n--- Example 1: Basic Monitoring ---" << std::endl;
    {
        NanoFlannMemory::MonitorConfig config;
        config.threshold_bytes = 10 * 1024 * 1024;  // 10MB threshold
        config.enable_periodic_reports = false;     // Disable for this example
        
        NanoFlannMemory::MemoryMonitor monitor(config);
        
        // Create a point cloud that should trigger the threshold
        PointCloud cloud(100000);  // 100k points
        
        // Use tracked allocator for tree building
        using TrackedAlloc = NanoFlannMemory::TrackedAllocator<Point3D>;
        MockKDTreeIndex<Point3D, TrackedAlloc> tree(cloud);
        
        std::cout << monitor.generate_report() << std::endl;
    }
    
    // Example 2: Monitoring with custom callbacks
    std::cout << "\n--- Example 2: Custom Callbacks ---" << std::endl;
    {
        NanoFlannMemory::MonitorConfig config;
        config.threshold_bytes = 5 * 1024 * 1024;   // 5MB threshold
        config.enable_periodic_reports = true;
        config.sampling_interval = std::chrono::milliseconds(50);
        config.threshold_callback = threshold_alert_callback;
        config.periodic_callback = periodic_report_callback;
        config.log_file_path = "memory_monitor.log";
        
        NanoFlannMemory::MemoryMonitor monitor(config);
        
        // Create multiple trees to show progressive memory usage
        PointCloud cloud1(50000);
        PointCloud cloud2(75000);
        
        using TrackedAlloc = NanoFlannMemory::TrackedAllocator<Point3D>;
        
        std::cout << "Building first tree..." << std::endl;
        MockKDTreeIndex<Point3D, TrackedAlloc> tree1(cloud1);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        std::cout << "Building second tree..." << std::endl;
        MockKDTreeIndex<Point3D, TrackedAlloc> tree2(cloud2);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        std::cout << monitor.generate_report() << std::endl;
    }
    
    // Example 3: Detailed tracking mode
    std::cout << "\n--- Example 3: Detailed Tracking ---" << std::endl;
    {
        NanoFlannMemory::MonitorConfig config;
        config.threshold_bytes = 15 * 1024 * 1024;  // 15MB threshold
        config.enable_detailed_tracking = true;     // Enable detailed tracking
        config.enable_periodic_reports = false;
        
        NanoFlannMemory::MemoryMonitor monitor(config);
        
        PointCloud cloud(80000);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        using TrackedAlloc = NanoFlannMemory::TrackedAllocator<Point3D>;
        MockKDTreeIndex<Point3D, TrackedAlloc> tree(cloud);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "Tree building took: " << duration.count() << " ms" << std::endl;
        std::cout << monitor.generate_report() << std::endl;
        
        // Reset and build another tree
        std::cout << "\nResetting monitor and building another tree..." << std::endl;
        monitor.reset();
        
        PointCloud cloud2(60000);
        MockKDTreeIndex<Point3D, TrackedAlloc> tree2(cloud2);
        
        std::cout << monitor.generate_report() << std::endl;
    }
    
    // Example 4: Using convenience macros
    std::cout << "\n--- Example 4: Convenience Macros ---" << std::endl;
    {
        // Simple macro-based monitoring
        NANOFLANN_MONITOR_START(8);  // 8MB threshold
        
        PointCloud cloud(60000);
        
        using TrackedAlloc = NanoFlannMemory::TrackedAllocator<Point3D>;
        MockKDTreeIndex<Point3D, TrackedAlloc> tree(cloud);
        
        NANOFLANN_MONITOR_REPORT();
        
        // Build another tree
        PointCloud cloud2(40000);
        MockKDTreeIndex<Point3D, TrackedAlloc> tree2(cloud2);
        
        NANOFLANN_MONITOR_REPORT();
        
        NANOFLANN_MONITOR_RESET();
        NANOFLANN_MONITOR_REPORT();
    }
    
    // Example 5: Performance comparison
    std::cout << "\n--- Example 5: Performance Comparison ---" << std::endl;
    {
        const size_t n_points = 100000;
        
        // Without monitoring
        auto start = std::chrono::high_resolution_clock::now();
        PointCloud cloud1(n_points);
        MockKDTreeIndex<Point3D> tree_normal(cloud1);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration_normal = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // With monitoring
        NanoFlannMemory::MonitorConfig config;
        config.enable_periodic_reports = false;
        NanoFlannMemory::MemoryMonitor monitor(config);
        
        start = std::chrono::high_resolution_clock::now();
        PointCloud cloud2(n_points);
        using TrackedAlloc = NanoFlannMemory::TrackedAllocator<Point3D>;
        MockKDTreeIndex<Point3D, TrackedAlloc> tree_tracked(cloud2);
        end = std::chrono::high_resolution_clock::now();
        auto duration_tracked = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Performance comparison for " << n_points << " points:" << std::endl;
        std::cout << "Normal allocation: " << duration_normal.count() << " μs" << std::endl;
        std::cout << "Tracked allocation: " << duration_tracked.count() << " μs" << std::endl;
        std::cout << "Overhead: " << ((duration_tracked.count() - duration_normal.count()) * 100.0 / duration_normal.count()) << "%" << std::endl;
        
        std::cout << monitor.generate_report() << std::endl;
    }
    
    std::cout << "\n=== Example completed successfully! ===" << std::endl;
    
    return 0;
}
/**
 * Real Nanoflann Memory Monitor Example
 * 
 * This example demonstrates how to use the nanoflann memory monitor
 * with the actual nanoflann library for real tree building operations.
 * 
 * Features demonstrated:
 * - Real nanoflann KD-tree construction with memory monitoring
 * - Point cloud data generation and tree building
 * - Memory usage tracking during actual tree operations
 * - Performance comparison with and without monitoring
 */

#include <memory/nanoflann_memory_monitor.hpp>
#include <nanoflann.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>

// Point cloud structure for nanoflann
struct PointCloud {
    std::vector<std::vector<double>> pts;

    // Must return the number of data points
    inline size_t kdtree_get_point_count() const { return pts.size(); }

    // Returns the dim'th component of the idx'th point in the class
    inline double kdtree_get_pt(const size_t idx, const size_t dim) const {
        return pts[idx][dim];
    }

    // Optional bounding-box computation: return false to default to a standard bbox computation loop.
    template <class BBOX>
    bool kdtree_get_bbox(BBOX&) const { return false; }
};

// Generate random point cloud
PointCloud generate_point_cloud(size_t num_points, size_t dimension) {
    PointCloud cloud;
    cloud.pts.reserve(num_points);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(0.0, 1000.0);
    
    for (size_t i = 0; i < num_points; ++i) {
        std::vector<double> point(dimension);
        for (size_t j = 0; j < dimension; ++j) {
            point[j] = dis(gen);
        }
        cloud.pts.push_back(std::move(point));
    }
    
    return cloud;
}

// Build nanoflann tree with memory monitoring
template<typename PointCloud>
void build_tree_with_monitoring(const PointCloud& cloud, 
                               nanoflann::MemoryMonitor& monitor,
                               const std::string& context) {
    
    std::cout << "Building tree for " << cloud.kdtree_get_point_count() 
              << " points in " << (cloud.pts.empty() ? 0 : cloud.pts[0].size()) 
              << " dimensions..." << std::endl;
    
    // Use RAII scope for automatic tree build tracking
    nanoflann::TreeBuildScope scope(monitor, context);
    
    // Create and build the nanoflann tree
    using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<double, PointCloud>,
        PointCloud,
        3  // max leaf
    >;
    
    KDTree index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
    
    // This is where the actual tree building happens
    index.buildIndex();
    
    std::cout << "Tree built successfully!" << std::endl;
    
    // Perform a simple query to verify the tree works
    std::vector<double> query_pt(cloud.pts[0].size(), 500.0);
    std::vector<std::pair<size_t, double>> matches;
    
    index.radiusSearch(&query_pt[0], 100.0, matches, nanoflann::SearchParams());
    
    std::cout << "Found " << matches.size() << " points within radius 100.0" << std::endl;
}

// Performance test with and without monitoring
template<typename PointCloud>
void performance_comparison(const PointCloud& cloud) {
    std::cout << "\n=== Performance Comparison ===" << std::endl;
    
    // Test without monitoring
    auto start = std::chrono::high_resolution_clock::now();
    
    using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<double, PointCloud>,
        PointCloud,
        3
    >;
    
    KDTree index_no_monitor(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
    index_no_monitor.buildIndex();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_no_monitor = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Test with monitoring
    auto monitor = nanoflann::memory_utils::create_default_monitor();
    monitor.start();
    
    start = std::chrono::high_resolution_clock::now();
    
    KDTree index_with_monitor(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
    {
        nanoflann::TreeBuildScope scope(monitor, "Performance test");
        index_with_monitor.buildIndex();
    }
    
    end = std::chrono::high_resolution_clock::now();
    auto duration_with_monitor = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    monitor.stop();
    
    // Report results
    std::cout << "Build time without monitoring: " << duration_no_monitor.count() << " μs" << std::endl;
    std::cout << "Build time with monitoring:   " << duration_with_monitor.count() << " μs" << std::endl;
    
    double overhead_percent = ((duration_with_monitor.count() - duration_no_monitor.count()) / 
                              static_cast<double>(duration_no_monitor.count())) * 100.0;
    
    std::cout << "Monitoring overhead: " << std::fixed << std::setprecision(2) 
              << overhead_percent << "%" << std::endl;
}

int main() {
    std::cout << "=== Real Nanoflann Memory Monitor Example ===" << std::endl;
    
    // Create memory monitor with appropriate settings
    nanoflann::MemoryMonitor::Config config;
    config.memory_threshold_mb = 100;  // 100MB threshold
    config.check_interval_ms = 50;     // Check every 50ms
    config.enable_background_monitoring = true;
    config.enable_detailed_logging = true;
    config.log_prefix = "[NANOFLANN_REAL]";
    
    nanoflann::MemoryMonitor monitor(config);
    
    // Add standard logging
    nanoflann::memory_utils::add_standard_logging(monitor);
    
    // Add custom callback for detailed analysis
    monitor.add_callback([](const nanoflann::MemoryMonitor::MemoryEvent& event) {
        if (event.type == nanoflann::MemoryMonitor::EventType::THRESHOLD_EXCEEDED) {
            std::cout << "\n*** CRITICAL: Memory threshold exceeded! ***" << std::endl;
            std::cout << "Current memory: " << event.memory_mb << "MB" << std::endl;
            std::cout << "Context: " << event.context << std::endl;
        }
    });
    
    // Start monitoring
    monitor.start();
    
    std::cout << "Memory monitoring started with threshold: " 
              << monitor.get_threshold() << "MB" << std::endl;
    
    // Example 1: Small point cloud (3D)
    {
        std::cout << "\n--- Example 1: Small 3D Point Cloud (10K points) ---" << std::endl;
        
        auto cloud = generate_point_cloud(10000, 3);
        
        // Estimate memory usage
        size_t estimated_memory = nanoflann::memory_utils::estimate_tree_memory_usage(
            cloud.kdtree_get_point_count(), 3, sizeof(double));
        
        std::cout << "Estimated memory usage: " << estimated_memory << "MB" << std::endl;
        
        build_tree_with_monitoring(cloud, monitor, "Small 3D cloud (10K points)");
    }
    
    // Example 2: Medium point cloud (5D)
    {
        std::cout << "\n--- Example 2: Medium 5D Point Cloud (100K points) ---" << std::endl;
        
        auto cloud = generate_point_cloud(100000, 5);
        
        size_t estimated_memory = nanoflann::memory_utils::estimate_tree_memory_usage(
            cloud.kdtree_get_point_count(), 5, sizeof(double));
        
        std::cout << "Estimated memory usage: " << estimated_memory << "MB" << std::endl;
        
        build_tree_with_monitoring(cloud, monitor, "Medium 5D cloud (100K points)");
    }
    
    // Example 3: Large point cloud (10D) - might trigger threshold
    {
        std::cout << "\n--- Example 3: Large 10D Point Cloud (500K points) ---" << std::endl;
        
        auto cloud = generate_point_cloud(500000, 10);
        
        size_t estimated_memory = nanoflann::memory_utils::estimate_tree_memory_usage(
            cloud.kdtree_get_point_count(), 10, sizeof(double));
        
        std::cout << "Estimated memory usage: " << estimated_memory << "MB" << std::endl;
        
        build_tree_with_monitoring(cloud, monitor, "Large 10D cloud (500K points)");
    }
    
    // Example 4: Performance comparison
    {
        std::cout << "\n--- Example 4: Performance Comparison ---" << std::endl;
        
        auto cloud = generate_point_cloud(50000, 3);
        performance_comparison(cloud);
    }
    
    // Example 5: Multiple trees with memory tracking
    {
        std::cout << "\n--- Example 5: Multiple Trees ---" << std::endl;
        
        for (size_t i = 0; i < 3; ++i) {
            auto cloud = generate_point_cloud(20000 + i * 10000, 3);
            
            std::string context = "Multiple trees - tree " + std::to_string(i + 1);
            build_tree_with_monitoring(cloud, monitor, context);
        }
    }
    
    // Stop monitoring
    monitor.stop();
    
    // Print final statistics
    auto stats = monitor.get_stats();
    std::cout << "\n=== Final Memory Statistics ===" << std::endl;
    std::cout << "Peak memory usage: " << stats.peak_memory_mb << "MB" << std::endl;
    std::cout << "Current memory usage: " << stats.current_memory_mb << "MB" << std::endl;
    
    // Print event history
    auto events = monitor.get_event_history();
    std::cout << "\n=== Memory Events (" << events.size() << " events) ===" << std::endl;
    
    for (const auto& event : events) {
        std::string event_type;
        switch (event.type) {
            case nanoflann::MemoryMonitor::EventType::THRESHOLD_EXCEEDED:
                event_type = "THRESHOLD_EXCEEDED";
                break;
            case nanoflann::MemoryMonitor::EventType::TREE_BUILD_START:
                event_type = "TREE_BUILD_START";
                break;
            case nanoflann::MemoryMonitor::EventType::TREE_BUILD_END:
                event_type = "TREE_BUILD_END";
                break;
            case nanoflann::MemoryMonitor::EventType::MEMORY_SPIKE_DETECTED:
                event_type = "MEMORY_SPIKE";
                break;
            default:
                event_type = "UNKNOWN";
        }
        
        std::cout << "- [" << event_type << "] " << event.context 
                  << " (" << event.memory_mb << "MB)" << std::endl;
    }
    
    std::cout << "\nReal nanoflann example completed successfully!" << std::endl;
    
    return 0;
}
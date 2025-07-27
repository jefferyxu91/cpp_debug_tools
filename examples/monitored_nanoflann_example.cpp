/**
 * Monitored Nanoflann Example
 * 
 * This example demonstrates the new MonitoredKDTreeIndex class that
 * automatically tracks memory usage during tree building operations.
 * 
 * Features demonstrated:
 * - Automatic memory monitoring during tree construction
 * - Automatic warning messages when memory threshold is exceeded
 * - Custom logging and configuration options
 * - Smart memory threshold estimation
 * - Performance comparison with regular nanoflann
 */

#include <memory/nanoflann_monitored_index.hpp>
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

// Custom logger function
void custom_logger(const std::string& message) {
    std::cout << "[CUSTOM_LOGGER] " << message << std::endl;
}

int main() {
    std::cout << "=== Monitored Nanoflann Example ===" << std::endl;
    
    // Example 1: Basic monitored index with default settings
    {
        std::cout << "\n--- Example 1: Basic Monitored Index ---" << std::endl;
        
        auto cloud = generate_point_cloud(10000, 3);
        
        // Create monitored index with 50MB threshold
        using MonitoredKDTree = nanoflann::MonitoredKDTreeIndex<
            nanoflann::L2_Simple_Adaptor<double, PointCloud>,
            PointCloud, 3>;
        
        MonitoredKDTree::MonitorConfig config(50); // 50MB threshold
        MonitoredKDTree index(3, cloud, config);
        
        std::cout << "Building monitored tree with 10K points..." << std::endl;
        index.buildIndex(); // This will automatically monitor memory usage
        
        // Get memory statistics
        auto stats = index.get_memory_stats();
        std::cout << "Peak memory usage: " << stats.peak_memory_mb << "MB" << std::endl;
        
        // Test the tree
        std::vector<double> query_pt(3, 500.0);
        std::vector<std::pair<size_t, double>> matches;
        index.radiusSearch(&query_pt[0], 100.0, matches, nanoflann::SearchParams());
        std::cout << "Found " << matches.size() << " points within radius 100.0" << std::endl;
    }
    
    // Example 2: Monitored index with custom logger
    {
        std::cout << "\n--- Example 2: Custom Logger ---" << std::endl;
        
        auto cloud = generate_point_cloud(50000, 5);
        
        using MonitoredKDTree = nanoflann::MonitoredKDTreeIndex<
            nanoflann::L2_Simple_Adaptor<double, PointCloud>,
            PointCloud, -1>; // Dynamic dimension
        
        MonitoredKDTree::MonitorConfig config;
        config.memory_threshold_mb = 30; // Low threshold to trigger warnings
        config.custom_logger = custom_logger;
        config.context_prefix = "CustomLogger";
        
        MonitoredKDTree index(5, cloud, config);
        
        std::cout << "Building monitored tree with 50K points (5D)..." << std::endl;
        index.buildIndex(); // Will use custom logger for warnings
        
        auto stats = index.get_memory_stats();
        std::cout << "Peak memory usage: " << stats.peak_memory_mb << "MB" << std::endl;
    }
    
    // Example 3: Smart monitored index with automatic threshold estimation
    {
        std::cout << "\n--- Example 3: Smart Threshold Estimation ---" << std::endl;
        
        auto cloud = generate_point_cloud(100000, 3);
        
        // Use smart monitored index that estimates memory usage
        auto index = nanoflann::monitored_utils::create_smart_monitored_index<
            nanoflann::L2_Simple_Adaptor<double, PointCloud>,
            PointCloud, 3>(3, cloud, 1.5); // 1.5x safety factor
        
        std::cout << "Estimated threshold: " << index.get_memory_threshold() << "MB" << std::endl;
        std::cout << "Building smart monitored tree with 100K points..." << std::endl;
        
        index.buildIndex();
        
        auto stats = index.get_memory_stats();
        std::cout << "Peak memory usage: " << stats.peak_memory_mb << "MB" << std::endl;
    }
    
    // Example 4: Large dataset that might trigger warnings
    {
        std::cout << "\n--- Example 4: Large Dataset (Might Trigger Warnings) ---" << std::endl;
        
        auto cloud = generate_point_cloud(500000, 10);
        
        using MonitoredKDTree = nanoflann::MonitoredKDTreeIndex<
            nanoflann::L2_Simple_Adaptor<double, PointCloud>,
            PointCloud, -1>;
        
        // Set a low threshold to demonstrate warnings
        MonitoredKDTree::MonitorConfig config;
        config.memory_threshold_mb = 100; // Low threshold for large dataset
        config.context_prefix = "LargeDataset";
        
        MonitoredKDTree index(10, cloud, config);
        
        std::cout << "Building monitored tree with 500K points (10D)..." << std::endl;
        std::cout << "This might trigger memory warnings..." << std::endl;
        
        index.buildIndex();
        
        auto stats = index.get_memory_stats();
        std::cout << "Peak memory usage: " << stats.peak_memory_mb << "MB" << std::endl;
        
        // Print memory events
        auto events = index.get_memory_events();
        std::cout << "Memory events recorded: " << events.size() << std::endl;
        
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
            
            std::cout << "  - [" << event_type << "] " << event.context 
                      << " (" << event.memory_mb << "MB)" << std::endl;
        }
    }
    
    // Example 5: Performance comparison
    {
        std::cout << "\n--- Example 5: Performance Comparison ---" << std::endl;
        
        auto cloud = generate_point_cloud(50000, 3);
        
        // Test regular nanoflann
        using RegularKDTree = nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<double, PointCloud>,
            PointCloud, 3>;
        
        auto start = std::chrono::high_resolution_clock::now();
        RegularKDTree regular_index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
        regular_index.buildIndex();
        auto end = std::chrono::high_resolution_clock::now();
        auto regular_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Test monitored nanoflann
        using MonitoredKDTree = nanoflann::MonitoredKDTreeIndex<
            nanoflann::L2_Simple_Adaptor<double, PointCloud>,
            PointCloud, 3>;
        
        start = std::chrono::high_resolution_clock::now();
        MonitoredKDTree monitored_index(3, cloud, 200); // 200MB threshold
        monitored_index.buildIndex();
        end = std::chrono::high_resolution_clock::now();
        auto monitored_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Report results
        std::cout << "Regular nanoflann build time: " << regular_duration.count() << " μs" << std::endl;
        std::cout << "Monitored nanoflann build time: " << monitored_duration.count() << " μs" << std::endl;
        
        double overhead_percent = ((monitored_duration.count() - regular_duration.count()) / 
                                  static_cast<double>(regular_duration.count())) * 100.0;
        
        std::cout << "Monitoring overhead: " << std::fixed << std::setprecision(2) 
                  << overhead_percent << "%" << std::endl;
        
        // Verify both trees work correctly
        std::vector<double> query_pt(3, 500.0);
        std::vector<std::pair<size_t, double>> matches1, matches2;
        
        regular_index.radiusSearch(&query_pt[0], 100.0, matches1, nanoflann::SearchParams());
        monitored_index.radiusSearch(&query_pt[0], 100.0, matches2, nanoflann::SearchParams());
        
        std::cout << "Regular tree found: " << matches1.size() << " points" << std::endl;
        std::cout << "Monitored tree found: " << matches2.size() << " points" << std::endl;
        std::cout << "Results match: " << (matches1.size() == matches2.size() ? "YES" : "NO") << std::endl;
    }
    
    // Example 6: Disabling monitoring
    {
        std::cout << "\n--- Example 6: Disabled Monitoring ---" << std::endl;
        
        auto cloud = generate_point_cloud(10000, 3);
        
        using MonitoredKDTree = nanoflann::MonitoredKDTreeIndex<
            nanoflann::L2_Simple_Adaptor<double, PointCloud>,
            PointCloud, 3>;
        
        MonitoredKDTree::MonitorConfig config;
        config.enable_auto_monitoring = false; // Disable monitoring
        
        MonitoredKDTree index(3, cloud, config);
        
        std::cout << "Building tree with monitoring disabled..." << std::endl;
        index.buildIndex(); // No monitoring will occur
        
        auto stats = index.get_memory_stats();
        std::cout << "Memory stats (should be zero): " << stats.peak_memory_mb << "MB" << std::endl;
    }
    
    // Example 7: Runtime configuration changes
    {
        std::cout << "\n--- Example 7: Runtime Configuration ---" << std::endl;
        
        auto cloud = generate_point_cloud(20000, 3);
        
        using MonitoredKDTree = nanoflann::MonitoredKDTreeIndex<
            nanoflann::L2_Simple_Adaptor<double, PointCloud>,
            PointCloud, 3>;
        
        MonitoredKDTree index(3, cloud, 50); // 50MB threshold
        
        std::cout << "Initial threshold: " << index.get_memory_threshold() << "MB" << std::endl;
        
        // Change threshold at runtime
        index.set_memory_threshold(25);
        std::cout << "New threshold: " << index.get_memory_threshold() << "MB" << std::endl;
        
        // Change context prefix
        index.set_context_prefix("RuntimeConfig");
        
        std::cout << "Building tree with runtime configuration..." << std::endl;
        index.buildIndex();
        
        auto stats = index.get_memory_stats();
        std::cout << "Peak memory usage: " << stats.peak_memory_mb << "MB" << std::endl;
    }
    
    std::cout << "\n=== Example completed successfully! ===" << std::endl;
    std::cout << "\nKey benefits of MonitoredKDTreeIndex:" << std::endl;
    std::cout << "1. Automatic memory monitoring during tree building" << std::endl;
    std::cout << "2. Automatic warning messages when thresholds are exceeded" << std::endl;
    std::cout << "3. Custom logging and configuration options" << std::endl;
    std::cout << "4. Smart memory threshold estimation" << std::endl;
    std::cout << "5. Minimal performance overhead" << std::endl;
    std::cout << "6. Drop-in replacement for regular nanoflann KDTreeSingleIndexAdaptor" << std::endl;
    
    return 0;
}
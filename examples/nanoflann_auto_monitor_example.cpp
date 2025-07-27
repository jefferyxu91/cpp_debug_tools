/**
 * Nanoflann Automatic Memory Monitor Example
 * 
 * This example demonstrates how to use the MonitoredKDTree classes that
 * automatically monitor memory usage during tree operations without
 * requiring any manual monitoring code.
 */

#include <memory/nanoflann_monitor_wrapper.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>

// Simple point cloud adaptor for 3D points
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

// Generate random point cloud
template <typename T>
void generateRandomPointCloud(PointCloud<T>& cloud, size_t num_points) {
    cloud.pts.resize(num_points * 3);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-1000.0, 1000.0);
    
    for (size_t i = 0; i < num_points * 3; ++i) {
        cloud.pts[i] = dis(gen);
    }
}

int main() {
    std::cout << "Nanoflann Automatic Memory Monitor Example" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    // Example 1: Basic usage with automatic monitoring
    std::cout << "\n=== Example 1: Basic Automatic Monitoring ===" << std::endl;
    {
        PointCloud<double> cloud;
        generateRandomPointCloud(cloud, 2000000);  // 2M points
        
        // Instead of using nanoflann::KDTreeSingleIndexAdaptor directly,
        // use the monitored version which automatically tracks memory
        using MonitoredKDTree = NanoflannMonitor::MonitoredKDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<double, PointCloud<double>>,
            PointCloud<double>,
            3  // dimensions
        >;
        
        // Configure monitoring (50MB threshold)
        NanoflannMonitor::MonitorConfig config;
        config.threshold_mb = 50;
        config.check_interval_ms = 50;
        
        // Create tree with monitoring
        MonitoredKDTree index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10), config, "Example1_Tree");
        
        // Building the index will automatically trigger monitoring
        std::cout << "\nBuilding KD-tree..." << std::endl;
        index.buildIndex();
        // Memory monitoring output appears automatically when threshold is exceeded
        
        // Perform some queries (no monitoring for queries as they don't allocate much)
        const size_t num_queries = 5;
        for (size_t i = 0; i < num_queries; ++i) {
            const double query_pt[3] = { 0.0, 0.0, 0.0 };
            const size_t num_results = 1;
            uint32_t ret_index;  // Use uint32_t to match IndexType
            double out_dist_sqr;
            
            index.knnSearch(&query_pt[0], num_results, &ret_index, &out_dist_sqr);
        }
    }
    
    // Example 2: Using the helper function for quick setup
    std::cout << "\n=== Example 2: Using Helper Function ===" << std::endl;
    {
        PointCloud<double> cloud;
        generateRandomPointCloud(cloud, 3000000);  // 3M points
        
        // Create a monitored tree with one line
        auto index = NanoflannMonitor::makeMonitoredKDTree<
            nanoflann::L2_Simple_Adaptor<double, PointCloud<double>>,
            PointCloud<double>,
            3
        >(3, cloud, 75, "QuickSetup_Tree");  // 75MB threshold
        
        std::cout << "\nBuilding KD-tree with helper..." << std::endl;
        index.buildIndex();
    }
    
    // Example 3: Dynamic tree with monitoring
    std::cout << "\n=== Example 3: Dynamic Tree Monitoring ===" << std::endl;
    {
        PointCloud<double> cloud;
        generateRandomPointCloud(cloud, 1000000);  // Start with 1M points
        
        using DynamicMonitoredTree = NanoflannMonitor::MonitoredKDTreeDynamicAdaptor<
            nanoflann::L2_Simple_Adaptor<double, PointCloud<double>>,
            PointCloud<double>,
            3
        >;
        
        NanoflannMonitor::MonitorConfig config;
        config.threshold_mb = 40;
        
        DynamicMonitoredTree index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10), 10000000, config, "Dynamic_Tree");
        
        std::cout << "\nInitial tree built automatically with existing points..." << std::endl;
        
        // Add more points (this will trigger monitoring if adding many points)
        std::cout << "\nAdding 50k more points..." << std::endl;
        size_t old_size = cloud.pts.size();
        cloud.pts.resize(old_size + 50000 * 3);
        
        // Generate new points
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(-1000.0, 1000.0);
        for (size_t i = old_size; i < cloud.pts.size(); ++i) {
            cloud.pts[i] = dis(gen);
        }
        
        // Add points to tree (monitored automatically for large additions)
        index.addPoints(old_size / 3, (old_size + 50000 * 3) / 3);
    }
    
    // Example 4: Customizing monitoring behavior
    std::cout << "\n=== Example 4: Custom Monitoring Configuration ===" << std::endl;
    {
        PointCloud<double> cloud;
        generateRandomPointCloud(cloud, 4000000);  // 4M points
        
        using MonitoredKDTree = NanoflannMonitor::MonitoredKDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<double, PointCloud<double>>,
            PointCloud<double>,
            3
        >;
        
        // Custom configuration
        NanoflannMonitor::MonitorConfig config;
        config.threshold_mb = 100;
        config.check_interval_ms = 100;  // Less frequent checks
        config.monitor_rss = true;
        config.monitor_vss = false;
        
        // Custom logger that could write to a file
        config.custom_logger = [](const std::string& message) {
            std::cout << "[CUSTOM OUTPUT] " << message << std::endl;
        };
        config.print_to_stderr = false;  // Disable default output
        
        MonitoredKDTree index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10), config, "Custom_Tree");
        
        std::cout << "\nBuilding large tree with custom logger..." << std::endl;
        index.buildIndex();
    }
    
    // Example 5: Comparing with non-monitored version
    std::cout << "\n=== Example 5: Performance Comparison ===" << std::endl;
    {
        PointCloud<double> cloud;
        generateRandomPointCloud(cloud, 1000000);
        
        // Timing with monitoring
        auto start = std::chrono::high_resolution_clock::now();
        {
            using MonitoredKDTree = NanoflannMonitor::MonitoredKDTreeSingleIndexAdaptor<
                nanoflann::L2_Simple_Adaptor<double, PointCloud<double>>,
                PointCloud<double>,
                3
            >;
            
            NanoflannMonitor::MonitorConfig config;
            config.threshold_mb = 200;  // High threshold to reduce output
            
            MonitoredKDTree index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10), config);
            index.buildIndex();
        }
        auto monitored_time = std::chrono::high_resolution_clock::now() - start;
        
        // Timing without monitoring
        start = std::chrono::high_resolution_clock::now();
        {
            using StandardKDTree = nanoflann::KDTreeSingleIndexAdaptor<
                nanoflann::L2_Simple_Adaptor<double, PointCloud<double>>,
                PointCloud<double>,
                3
            >;
            
            StandardKDTree index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
            index.buildIndex();
        }
        auto standard_time = std::chrono::high_resolution_clock::now() - start;
        
        std::cout << "\nPerformance comparison:" << std::endl;
        std::cout << "  With monitoring: " 
                  << std::chrono::duration_cast<std::chrono::milliseconds>(monitored_time).count() 
                  << " ms" << std::endl;
        std::cout << "  Without monitoring: " 
                  << std::chrono::duration_cast<std::chrono::milliseconds>(standard_time).count() 
                  << " ms" << std::endl;
        std::cout << "  Overhead: " 
                  << (100.0 * (monitored_time.count() - standard_time.count()) / standard_time.count())
                  << "%" << std::endl;
    }
    
    std::cout << "\nAll examples completed!" << std::endl;
    return 0;
}
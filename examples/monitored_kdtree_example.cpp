/**
 * Monitored KDTree Example
 * 
 * This example demonstrates the MonitoredKDTreeSingleIndexAdaptor class
 * which is a drop-in replacement for nanoflann's KDTreeSingleIndexAdaptor
 * that automatically monitors memory usage and prints alerts when thresholds
 * are exceeded.
 */

#include <memory/nanoflann_memory_monitor.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <chrono>

// Simple 3D point structure
struct Point3D {
    double x, y, z;
};

// Point cloud adapter for nanoflann
struct PointCloud {
    std::vector<Point3D> pts;

    inline size_t kdtree_get_point_count() const { return pts.size(); }
    inline double kdtree_get_pt(const size_t idx, const size_t dim) const {
        if (dim == 0) return pts[idx].x;
        else if (dim == 1) return pts[idx].y;
        else return pts[idx].z;
    }
    template <class BBOX>
    bool kdtree_get_bbox(BBOX& /*bb*/) const { return false; }
};

void generate_random_cloud(PointCloud& cloud, size_t N, double range = 100.0) {
    cloud.pts.resize(N);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(-range, range);
    
    for (size_t i = 0; i < N; i++) {
        cloud.pts[i].x = dis(gen);
        cloud.pts[i].y = dis(gen);
        cloud.pts[i].z = dis(gen);
    }
}

int main() {
    std::cout << "=== Monitored KDTree Example ===" << std::endl;
    std::cout << "This demonstrates automatic memory monitoring with derived nanoflann classes.\n" << std::endl;
    
    // Example 1: Basic usage with default monitoring
    std::cout << "--- Example 1: Basic MonitoredKDTree (default 50MB threshold) ---" << std::endl;
    {
        PointCloud cloud;
        generate_random_cloud(cloud, 100000);
        
        // This is a drop-in replacement for nanoflann::KDTreeSingleIndexAdaptor
        // but automatically monitors memory and prints alerts
        NanoFlannMemory::MonitoredKDTree3D<double, PointCloud> index(
            3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10)
        );
        
        std::cout << "Construction completed. Memory used: " 
                  << (index.get_construction_memory_usage() / (1024.0 * 1024.0)) << " MB" << std::endl;
        
        // The tree works exactly like regular nanoflann
        double query_pt[3] = {1.0, 2.0, 3.0};
        size_t ret_index;
        double out_dist_sqr;
        nanoflann::KNNResultSet<double> resultSet(1);
        resultSet.init(&ret_index, &out_dist_sqr);
        index.findNeighbors(resultSet, &query_pt[0]);
        
        std::cout << "Nearest neighbor: index=" << ret_index << ", distance²=" << out_dist_sqr << std::endl;
    }
    
    // Example 2: Custom threshold with alert demonstration
    std::cout << "\n--- Example 2: Custom 5MB Threshold (will likely trigger alert) ---" << std::endl;
    {
        PointCloud cloud;
        generate_random_cloud(cloud, 200000);  // Larger dataset
        
        // Configure monitoring with low threshold to trigger alert
        NanoFlannMemory::MonitorConfig config;
        config.threshold_bytes = 5 * 1024 * 1024;  // 5MB threshold (low to trigger alert)
        config.auto_print_on_exceed = true;        // Auto-print when exceeded
        config.print_construction_summary = true;  // Print construction summary
        
        NanoFlannMemory::MonitoredKDTree3D<double, PointCloud> index(
            3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10), config
        );
        
        if (index.memory_threshold_exceeded()) {
            std::cout << "As expected, the 5MB threshold was exceeded!" << std::endl;
        }
    }
    
    // Example 3: Quiet mode - no automatic printing
    std::cout << "\n--- Example 3: Quiet Mode (no automatic output) ---" << std::endl;
    {
        PointCloud cloud;
        generate_random_cloud(cloud, 150000);
        
        NanoFlannMemory::MonitorConfig config;
        config.threshold_bytes = 10 * 1024 * 1024;  // 10MB threshold
        config.auto_print_on_exceed = false;        // Don't auto-print
        config.print_construction_summary = false;  // Don't print summary
        
        NanoFlannMemory::MonitoredKDTree3D<double, PointCloud> index(
            3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10), config
        );
        
        std::cout << "Tree built silently. You can manually check status:" << std::endl;
        std::cout << "Memory used: " << (index.get_construction_memory_usage() / (1024.0 * 1024.0)) << " MB" << std::endl;
        std::cout << "Threshold exceeded: " << (index.memory_threshold_exceeded() ? "Yes" : "No") << std::endl;
        
        // Manual memory report
        std::cout << "\nManual memory report:" << std::endl;
        index.print_memory_status();
    }
    
    // Example 4: Custom callback for threshold alerts
    std::cout << "\n--- Example 4: Custom Threshold Callback ---" << std::endl;
    {
        PointCloud cloud;
        generate_random_cloud(cloud, 180000);
        
        NanoFlannMemory::MonitorConfig config;
        config.threshold_bytes = 8 * 1024 * 1024;  // 8MB threshold
        config.auto_print_on_exceed = false;       // Disable default printing
        config.print_construction_summary = true;
        
        // Custom callback for when threshold is exceeded
        config.threshold_callback = [](size_t usage, const std::string& /*message*/) {
            std::cout << "🚨 CUSTOM ALERT: High memory usage detected!" << std::endl;
            std::cout << "   Current usage: " << (usage / (1024.0 * 1024.0)) << " MB" << std::endl;
            std::cout << "   Recommendation: Consider reducing dataset size or increasing threshold." << std::endl;
        };
        
        NanoFlannMemory::MonitoredKDTree3D<double, PointCloud> index(
            3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10), config
        );
    }
    
    // Example 5: Performance comparison with normal nanoflann
    std::cout << "\n--- Example 5: Performance Comparison ---" << std::endl;
    {
        const size_t N = 100000;
        PointCloud cloud;
        generate_random_cloud(cloud, N);
        
        // Time normal nanoflann
        auto start = std::chrono::high_resolution_clock::now();
        nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<double, PointCloud>,
            PointCloud,
            3
        > normal_index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
        auto end = std::chrono::high_resolution_clock::now();
        auto normal_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Time monitored nanoflann
        NanoFlannMemory::MonitorConfig config;
        config.print_construction_summary = false;  // Quiet for timing
        
        start = std::chrono::high_resolution_clock::now();
        NanoFlannMemory::MonitoredKDTree3D<double, PointCloud> monitored_index(
            3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10), config
        );
        end = std::chrono::high_resolution_clock::now();
        auto monitored_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Performance comparison for " << N << " points:" << std::endl;
        std::cout << "  Normal nanoflann:    " << normal_time.count() << " ms" << std::endl;
        std::cout << "  Monitored nanoflann: " << monitored_time.count() << " ms" << std::endl;
        
        if (monitored_time.count() > 0) {
            double overhead = ((double)(monitored_time.count() - normal_time.count()) / normal_time.count()) * 100.0;
            std::cout << "  Monitoring overhead: " << std::fixed << std::setprecision(2) << overhead << "%" << std::endl;
        }
        
        std::cout << "  Memory used: " << (monitored_index.get_construction_memory_usage() / (1024.0 * 1024.0)) << " MB" << std::endl;
    }
    
    // Example 6: Using type aliases for convenience
    std::cout << "\n--- Example 6: Using Convenience Type Aliases ---" << std::endl;
    {
        PointCloud cloud;
        generate_random_cloud(cloud, 50000);
        
        // Using the convenience alias - much cleaner!
        NanoFlannMemory::MonitoredKDTree3D<float, PointCloud> index(
            3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(20)
        );
        
        std::cout << "Used MonitoredKDTree3D alias for clean, simple code!" << std::endl;
        std::cout << "Memory used: " << (index.get_construction_memory_usage() / (1024.0 * 1024.0)) << " MB" << std::endl;
    }
    
    std::cout << "\n=== Monitored KDTree Example Completed! ===" << std::endl;
    std::cout << "Key benefits of MonitoredKDTreeSingleIndexAdaptor:" << std::endl;
    std::cout << "✓ Drop-in replacement for nanoflann::KDTreeSingleIndexAdaptor" << std::endl;
    std::cout << "✓ Automatic memory monitoring and threshold alerts" << std::endl;
    std::cout << "✓ Configurable reporting and callback options" << std::endl;
    std::cout << "✓ Minimal performance overhead" << std::endl;
    std::cout << "✓ Works with all existing nanoflann functionality" << std::endl;
    
    return 0;
}
/**
 * Simple Integration Example
 * 
 * This example shows the minimal changes needed to add memory monitoring
 * to existing nanoflann code. Perfect for quick integration testing.
 */

#include <memory/nanoflann_memory_monitor.hpp>
#include <nanoflann.hpp>
#include <iostream>
#include <vector>
#include <random>

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

int main() {
    std::cout << "=== Simple NanoFlann Memory Monitor Integration ===" << std::endl;
    
    // Step 1: Configure and start monitoring with custom settings
    NanoFlannMemory::MonitorConfig config;
    config.threshold_bytes = 25 * 1024 * 1024;  // 25MB threshold
    config.enable_periodic_reports = false;
    
    NanoFlannMemory::MemoryMonitor monitor(config);
    std::cout << "Memory monitoring started with 25MB threshold" << std::endl;
    std::cout << monitor.generate_report() << std::endl;
    
    // Step 2: Generate some test data (your existing code)
    const size_t N = 100000;
    PointCloud cloud;
    cloud.pts.resize(N);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(-50.0, 50.0);
    
    for (size_t i = 0; i < N; i++) {
        cloud.pts[i].x = dis(gen);
        cloud.pts[i].y = dis(gen);
        cloud.pts[i].z = dis(gen);
    }
    
    std::cout << "Generated " << N << " random points" << std::endl;
    std::cout << "Memory after point generation:" << std::endl;
    std::cout << monitor.generate_report() << std::endl;
    
    // Step 3: Your KDTree typedef remains the same - no changes needed!
    using MyKDTree = nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<double, PointCloud>,
        PointCloud,
        3  // dimensionality
    >;
    
    // Step 4: Build your KD-tree (your existing code - no changes needed!)
    std::cout << "Building KD-tree..." << std::endl;
    MyKDTree index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
    
    // Step 5: Check memory usage after tree building
    std::cout << "Memory after KD-tree construction:" << std::endl;
    std::cout << monitor.generate_report() << std::endl;
    
    if (monitor.threshold_exceeded()) {
        std::cout << "⚠️  Memory threshold was exceeded during tree building!" << std::endl;
    } else {
        std::cout << "✅ Memory usage stayed within threshold." << std::endl;
    }
    
    // Step 6: Continue with your normal nanoflann usage
    std::cout << "Performing some searches..." << std::endl;
    
    for (int i = 0; i < 1000; i++) {
        double query_pt[3] = {dis(gen), dis(gen), dis(gen)};
        
        // Find nearest neighbor
        size_t ret_index;
        double out_dist_sqr;
        nanoflann::KNNResultSet<double> resultSet(1);
        resultSet.init(&ret_index, &out_dist_sqr);
        index.findNeighbors(resultSet, &query_pt[0]);
    }
    
    std::cout << "Searches completed" << std::endl;
    
    // Step 7: Check final memory usage
    std::cout << "Final memory usage:" << std::endl;
    std::cout << monitor.generate_report() << std::endl;
    
    std::cout << "\n=== Integration Example Completed Successfully! ===" << std::endl;
    std::cout << "Integration is simple - just add memory monitoring around your nanoflann code:" << std::endl;
    std::cout << "1. Create a MemoryMonitor with desired threshold" << std::endl;
    std::cout << "2. Use monitor.generate_report() to check memory usage" << std::endl;
    std::cout << "3. Your existing nanoflann code requires no changes!" << std::endl;
    std::cout << "\nThe monitor tracks process memory usage automatically." << std::endl;
    
    return 0;
}
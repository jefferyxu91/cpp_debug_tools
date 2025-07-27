/**
 * Real NanoFlann Memory Monitor Example
 * 
 * This example demonstrates how to use the memory monitoring tool
 * with the actual nanoflann library to track memory usage during 
 * real KD-tree building and nearest neighbor search operations.
 * 
 * Features demonstrated:
 * - Integration with real nanoflann KDTreeSingleIndexAdaptor
 * - Memory tracking during tree construction
 * - Memory monitoring during search operations
 * - Performance impact analysis
 * - Real-world usage patterns
 */

#include <memory/nanoflann_memory_monitor.hpp>
#include <nanoflann.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <chrono>
#include <cstdlib>

// Point cloud adapter for nanoflann
template <typename T>
struct PointCloud {
    struct Point {
        T x, y, z;
    };

    std::vector<Point> pts;

    // Must return the number of data points
    inline size_t kdtree_get_point_count() const { return pts.size(); }

    // Returns the dim'th component of the idx'th point in the class:
    inline T kdtree_get_pt(const size_t idx, const size_t dim) const {
        if (dim == 0) return pts[idx].x;
        else if (dim == 1) return pts[idx].y;
        else return pts[idx].z;
    }

    // Optional bounding-box computation: return false to default to a standard bbox computation loop.
    template <class BBOX>
    bool kdtree_get_bbox(BBOX& /*bb*/) const { return false; }
};

// Generate random point cloud
template <typename T>
void generate_random_pointcloud(PointCloud<T>& cloud, const size_t N, const T max_range = 10) {
    cloud.pts.resize(N);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<T> dis(-max_range, max_range);
    
    for (size_t i = 0; i < N; i++) {
        cloud.pts[i].x = dis(gen);
        cloud.pts[i].y = dis(gen);
        cloud.pts[i].z = dis(gen);
    }
}

// Custom callback functions
void memory_threshold_callback(size_t current_usage, const std::string& message) {
    std::cout << "🚨 MEMORY ALERT: " << message << std::endl;
    std::cout << "   Consider optimizing your point cloud size or algorithm parameters." << std::endl;
}

void periodic_memory_callback(size_t current, size_t peak, size_t alloc_count) {
    static int counter = 0;
    if (++counter % 20 == 0) {  // Report every 20 cycles (2 seconds with 100ms interval)
        std::cout << "📊 [PERIODIC] Memory: " 
                  << std::fixed << std::setprecision(2)
                  << (current / (1024.0 * 1024.0)) << " MB current, "
                  << (peak / (1024.0 * 1024.0)) << " MB peak, "
                  << alloc_count << " allocations" << std::endl;
    }
}

int main() {
    std::cout << "=== Real NanoFlann Memory Monitor Example ===" << std::endl;
    
    // Example 1: Basic KD-tree with memory monitoring
    std::cout << "\n--- Example 1: Basic KD-tree Construction ---" << std::endl;
    {
        using num_t = double;
        using PointCloudType = PointCloud<num_t>;
        
        // Configure memory monitoring
        NanoFlannMemory::MonitorConfig config;
        config.threshold_bytes = 20 * 1024 * 1024;  // 20MB threshold
        config.enable_periodic_reports = false;
        config.enable_detailed_tracking = true;     // For accurate tracking
        
        NanoFlannMemory::MemoryMonitor monitor(config);
        
        // Generate point cloud
        const size_t num_points = 100000;
        PointCloudType cloud;
        
        std::cout << "Generating " << num_points << " random points..." << std::endl;
        generate_random_pointcloud(cloud, num_points);
        
        // Create KD-tree index with tracked allocator
        using TrackedAlloc = NanoFlannMemory::TrackedAllocator<std::pair<size_t, num_t>>;
        using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<num_t, PointCloudType>,
            PointCloudType,
            3, /* dim */
            TrackedAlloc
        >;
        
        std::cout << "Building KD-tree..." << std::endl;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        KDTree index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10 /* max leaf */));
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto build_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "KD-tree built in " << build_time.count() << " ms" << std::endl;
        std::cout << monitor.generate_report() << std::endl;
        
        // Perform some searches to see if there are additional allocations
        std::cout << "Performing 1000 nearest neighbor searches..." << std::endl;
        monitor.reset();
        
        auto search_start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < 1000; ++i) {
            num_t query_pt[3] = {
                static_cast<num_t>(rand() % 20 - 10),
                static_cast<num_t>(rand() % 20 - 10),
                static_cast<num_t>(rand() % 20 - 10)
            };
            
            size_t ret_index;
            num_t out_dist_sqr;
            nanoflann::KNNResultSet<num_t> resultSet(1);
            resultSet.init(&ret_index, &out_dist_sqr);
            index.findNeighbors(resultSet, &query_pt[0], nanoflann::SearchParams(10));
        }
        
        auto search_end = std::chrono::high_resolution_clock::now();
        auto search_time = std::chrono::duration_cast<std::chrono::milliseconds>(search_end - search_start);
        
        std::cout << "Searches completed in " << search_time.count() << " ms" << std::endl;
        std::cout << "Memory usage during searches:" << std::endl;
        std::cout << monitor.generate_report() << std::endl;
    }
    
    // Example 2: Large dataset with threshold monitoring
    std::cout << "\n--- Example 2: Large Dataset with Alerts ---" << std::endl;
    {
        using num_t = float;
        using PointCloudType = PointCloud<num_t>;
        
        // Configure with lower threshold and callbacks
        NanoFlannMemory::MonitorConfig config;
        config.threshold_bytes = 50 * 1024 * 1024;  // 50MB threshold
        config.enable_periodic_reports = true;
        config.sampling_interval = std::chrono::milliseconds(100);
        config.threshold_callback = memory_threshold_callback;
        config.periodic_callback = periodic_memory_callback;
        config.enable_detailed_tracking = false;    // Lightweight for large dataset
        config.log_file_path = "nanoflann_memory.log";
        
        NanoFlannMemory::MemoryMonitor monitor(config);
        
        // Large point cloud
        const size_t num_points = 500000;  // Half a million points
        PointCloudType cloud;
        
        std::cout << "Generating " << num_points << " random points..." << std::endl;
        generate_random_pointcloud(cloud, num_points);
        
        // Create KD-tree with tracked allocator
        using TrackedAlloc = NanoFlannMemory::TrackedAllocator<std::pair<size_t, num_t>>;
        using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<num_t, PointCloudType>,
            PointCloudType,
            3,
            TrackedAlloc
        >;
        
        std::cout << "Building large KD-tree (this may trigger memory alerts)..." << std::endl;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        KDTree index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto build_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "Large KD-tree built in " << build_time.count() << " ms" << std::endl;
        
        // Let periodic monitoring run for a bit
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::cout << monitor.generate_report() << std::endl;
    }
    
    // Example 3: Performance comparison
    std::cout << "\n--- Example 3: Performance Impact Analysis ---" << std::endl;
    {
        using num_t = double;
        using PointCloudType = PointCloud<num_t>;
        
        const size_t num_points = 200000;
        const int num_searches = 5000;
        
        // Generate test data
        PointCloudType cloud;
        generate_random_pointcloud(cloud, num_points);
        
        // Test 1: Without memory monitoring
        std::cout << "Building KD-tree WITHOUT memory monitoring..." << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        
        using NormalKDTree = nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<num_t, PointCloudType>,
            PointCloudType,
            3
        >;
        NormalKDTree normal_index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
        
        auto end = std::chrono::high_resolution_clock::now();
        auto normal_build_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Test 2: With memory monitoring
        std::cout << "Building KD-tree WITH memory monitoring..." << std::endl;
        NanoFlannMemory::MonitorConfig config;
        config.enable_periodic_reports = false;
        config.enable_detailed_tracking = false;  // Minimal overhead
        NanoFlannMemory::MemoryMonitor monitor(config);
        
        start = std::chrono::high_resolution_clock::now();
        
        using TrackedAlloc = NanoFlannMemory::TrackedAllocator<std::pair<size_t, num_t>>;
        using TrackedKDTree = nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<num_t, PointCloudType>,
            PointCloudType,
            3,
            TrackedAlloc
        >;
        TrackedKDTree tracked_index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
        
        end = std::chrono::high_resolution_clock::now();
        auto tracked_build_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Search performance comparison
        std::cout << "Performing " << num_searches << " searches (normal)..." << std::endl;
        start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_searches; ++i) {
            num_t query_pt[3] = {
                static_cast<num_t>(rand() % 20 - 10),
                static_cast<num_t>(rand() % 20 - 10),
                static_cast<num_t>(rand() % 20 - 10)
            };
            
            size_t ret_index;
            num_t out_dist_sqr;
            nanoflann::KNNResultSet<num_t> resultSet(1);
            resultSet.init(&ret_index, &out_dist_sqr);
            normal_index.findNeighbors(resultSet, &query_pt[0], nanoflann::SearchParams(10));
        }
        
        end = std::chrono::high_resolution_clock::now();
        auto normal_search_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Performing " << num_searches << " searches (tracked)..." << std::endl;
        start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_searches; ++i) {
            num_t query_pt[3] = {
                static_cast<num_t>(rand() % 20 - 10),
                static_cast<num_t>(rand() % 20 - 10),
                static_cast<num_t>(rand() % 20 - 10)
            };
            
            size_t ret_index;
            num_t out_dist_sqr;
            nanoflann::KNNResultSet<num_t> resultSet(1);
            resultSet.init(&ret_index, &out_dist_sqr);
            tracked_index.findNeighbors(resultSet, &query_pt[0], nanoflann::SearchParams(10));
        }
        
        end = std::chrono::high_resolution_clock::now();
        auto tracked_search_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Performance summary
        std::cout << "\n📈 Performance Analysis for " << num_points << " points:" << std::endl;
        std::cout << "Build time comparison:" << std::endl;
        std::cout << "  Normal:  " << normal_build_time.count() << " μs" << std::endl;
        std::cout << "  Tracked: " << tracked_build_time.count() << " μs" << std::endl;
        std::cout << "  Overhead: " << std::fixed << std::setprecision(2)
                  << ((tracked_build_time.count() - normal_build_time.count()) * 100.0 / normal_build_time.count()) 
                  << "%" << std::endl;
        
        std::cout << "\nSearch time comparison (" << num_searches << " searches):" << std::endl;
        std::cout << "  Normal:  " << normal_search_time.count() << " μs" << std::endl;
        std::cout << "  Tracked: " << tracked_search_time.count() << " μs" << std::endl;
        std::cout << "  Overhead: " << std::fixed << std::setprecision(2)
                  << ((tracked_search_time.count() - normal_search_time.count()) * 100.0 / normal_search_time.count()) 
                  << "%" << std::endl;
        
        std::cout << "\nMemory usage during tracked operations:" << std::endl;
        std::cout << monitor.generate_report() << std::endl;
    }
    
    // Example 4: Memory-aware parameter tuning
    std::cout << "\n--- Example 4: Memory-Aware Parameter Tuning ---" << std::endl;
    {
        using num_t = float;
        using PointCloudType = PointCloud<num_t>;
        
        const size_t num_points = 300000;
        PointCloudType cloud;
        generate_random_pointcloud(cloud, num_points);
        
        std::cout << "Testing different max_leaf parameters for memory usage..." << std::endl;
        
        std::vector<size_t> leaf_sizes = {5, 10, 20, 50, 100};
        
        for (size_t max_leaf : leaf_sizes) {
            NanoFlannMemory::MonitorConfig config;
            config.enable_periodic_reports = false;
            config.enable_detailed_tracking = true;
            NanoFlannMemory::MemoryMonitor monitor(config);
            
            using TrackedAlloc = NanoFlannMemory::TrackedAllocator<std::pair<size_t, num_t>>;
            using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
                nanoflann::L2_Simple_Adaptor<num_t, PointCloudType>,
                PointCloudType,
                3,
                TrackedAlloc
            >;
            
            auto start = std::chrono::high_resolution_clock::now();
            KDTree index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(max_leaf));
            auto end = std::chrono::high_resolution_clock::now();
            
            auto build_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            auto stats = monitor.get_stats();
            
            std::cout << "  max_leaf=" << max_leaf 
                      << ": " << std::fixed << std::setprecision(1)
                      << (stats.peak_usage.load() / (1024.0 * 1024.0)) << " MB, "
                      << build_time.count() << " ms, "
                      << stats.allocation_count.load() << " allocations" << std::endl;
        }
    }
    
    std::cout << "\n=== Real NanoFlann Example completed successfully! ===" << std::endl;
    std::cout << "Check 'nanoflann_memory.log' for detailed memory logs." << std::endl;
    
    return 0;
}
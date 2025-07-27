#include <iostream>
#include <vector>
#include <array>
#include <random>
#include <chrono>
#include "../include/memory/nanoflann_debug/nanoflann_memory_monitor.hpp"

// Simple dataset adaptor for 3D points
struct PointCloudAdaptor {
    const std::vector<std::array<float, 3>>& points;
    
    explicit PointCloudAdaptor(const std::vector<std::array<float, 3>>& pts) 
        : points(pts) {}
    
    inline size_t kdtree_get_point_count() const { return points.size(); }
    inline float kdtree_get_pt(const size_t idx, const size_t dim) const {
        return points[idx][dim];
    }
    template <class BBOX>
    bool kdtree_get_bbox(BBOX& /*bb*/) const { return false; }
};

int main() {
    std::cout << "Nanoflann Memory Monitor Demo\n";
    std::cout << "=============================\n\n";

    // Generate test data
    std::cout << "1. Generating test data...\n";
    std::vector<std::array<float, 3>> points;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-100.0f, 100.0f);
    
    const size_t num_points = 1000;
    points.reserve(num_points);
    for (size_t i = 0; i < num_points; ++i) {
        points.push_back({dis(gen), dis(gen), dis(gen)});
    }
    std::cout << "   Generated " << num_points << " random 3D points\n\n";

    PointCloudAdaptor dataset(points);

    // Demo 1: Normal operation with sufficient memory
    std::cout << "2. Demo 1: Normal operation (100MB threshold)\n";
    try {
        const size_t memory_threshold = 100 * 1024 * 1024; // 100 MB
        std::cout << "   Setting memory threshold to: " << memory_threshold / (1024*1024) << " MB\n";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        nanoflann::MemoryMonitoredKDTree<
            nanoflann::L2_Simple_Adaptor<float, PointCloudAdaptor, float, uint32_t>,
            PointCloudAdaptor,
            3,
            uint32_t> index(3, dataset, memory_threshold);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "   ✓ KD-tree built successfully in " << duration.count() << " ms\n";
        std::cout << "   ✓ Current memory usage: " << index.getCurrentMemoryUsage() / 1024 << " KB\n";
        
        // Test search functionality
        std::array<float, 3> query = {0.0f, 0.0f, 0.0f};
        std::vector<uint32_t> indices(5);
        std::vector<float> distances(5);
        
        size_t num_found = index.knnSearch(query.data(), 5, indices.data(), distances.data());
        std::cout << "   ✓ Found " << num_found << " nearest neighbors\n\n";
        
    } catch (const nanoflann::MemoryLimitExceededException& e) {
        std::cout << "   ✗ Memory limit exceeded: " << e.what() << "\n\n";
    }

    // Demo 2: Memory limit exceeded
    std::cout << "3. Demo 2: Memory limit exceeded (1MB threshold)\n";
    try {
        const size_t memory_threshold = 1024 * 1024; // 1 MB
        std::cout << "   Setting memory threshold to: " << memory_threshold / (1024*1024) << " MB\n";
        
        nanoflann::MemoryMonitoredKDTree<
            nanoflann::L2_Simple_Adaptor<float, PointCloudAdaptor, float, uint32_t>,
            PointCloudAdaptor,
            3,
            uint32_t> index(3, dataset, memory_threshold);
        
        std::cout << "   ✗ This should not be reached\n\n";
        
    } catch (const nanoflann::MemoryLimitExceededException& e) {
        std::cout << "   ✓ Memory limit exceeded as expected: " << e.what() << "\n\n";
    }

    // Demo 3: Custom parameters
    std::cout << "4. Demo 3: Custom build parameters\n";
    try {
        const size_t memory_threshold = 50 * 1024 * 1024; // 50 MB
        std::cout << "   Setting memory threshold to: " << memory_threshold / (1024*1024) << " MB\n";
        
        nanoflann::KDTreeSingleIndexAdaptorParams params;
        params.leaf_max_size = 10;  // Smaller leaf size
        params.n_thread_build = 1;  // Single thread
        
        std::cout << "   Using custom parameters: leaf_max_size=" << params.leaf_max_size 
                  << ", n_thread_build=" << params.n_thread_build << "\n";
        
        nanoflann::MemoryMonitoredKDTree<
            nanoflann::L2_Simple_Adaptor<float, PointCloudAdaptor, float, uint32_t>,
            PointCloudAdaptor,
            3,
            uint32_t> index(3, dataset, params, memory_threshold);
        
        std::cout << "   ✓ KD-tree built with custom parameters\n";
        std::cout << "   ✓ Current memory usage: " << index.getCurrentMemoryUsage() / 1024 << " KB\n\n";
        
    } catch (const nanoflann::MemoryLimitExceededException& e) {
        std::cout << "   ✗ Memory limit exceeded: " << e.what() << "\n\n";
    }

    std::cout << "Summary:\n";
    std::cout << "========\n";
    std::cout << "• The nanoflann memory monitor prevents excessive memory usage during KD-tree construction\n";
    std::cout << "• It throws MemoryLimitExceededException when the threshold is exceeded\n";
    std::cout << "• Memory monitoring has minimal overhead and integrates seamlessly with nanoflann\n";
    std::cout << "• Use it when building large KD-trees in memory-constrained environments\n";

    return 0;
}
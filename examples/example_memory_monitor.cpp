/**
 * Example demonstrating the memory-monitored nanoflann extension
 * 
 * This example shows how to use the MemoryMonitoredKDTree class to build
 * a KD-tree with memory monitoring capabilities.
 */

#include "../include/memory/nanoflann_debug/nanoflann_memory_monitor.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>

// Dataset adaptor for a vector of 3D points
struct PointCloudAdaptor {
    const std::vector<std::array<float, 3>>& points;
    
    explicit PointCloudAdaptor(const std::vector<std::array<float, 3>>& pts) 
        : points(pts) {}
    
    // Required interface for nanoflann
    inline size_t kdtree_get_point_count() const { return points.size(); }
    
    inline float kdtree_get_pt(const size_t idx, const size_t dim) const {
        return points[idx][dim];
    }
    
    template <class BBOX>
    bool kdtree_get_bbox(BBOX& /*bb*/) const { return false; }
};

int main() {
    std::cout << "Memory-Monitored nanoflann Example\n";
    std::cout << "===================================\n\n";
    
    // Generate some test data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1000.0f, 1000.0f);
    
    const size_t num_points = 10000; // 10 thousand points for testing
    std::vector<std::array<float, 3>> points;
    points.reserve(num_points);
    
    std::cout << "Generating " << num_points << " random 3D points...\n";
    for (size_t i = 0; i < num_points; ++i) {
        points.push_back({dist(gen), dist(gen), dist(gen)});
    }
    
    // Create dataset adaptor
    PointCloudAdaptor dataset(points);
    
    // Set memory threshold (e.g., 1 MB to trigger memory limit)
    const size_t memory_threshold = 1 * 1024 * 1024; // 1 MB in bytes
    
    std::cout << "Memory threshold set to: " << (memory_threshold / (1024 * 1024)) << " MB\n\n";
    
    try {
        // Create memory-monitored KD-tree
        std::cout << "Building memory-monitored KD-tree...\n";
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Use the proper distance adaptor
        using DistanceAdaptor = nanoflann::L2_Simple_Adaptor<float, PointCloudAdaptor, float, uint32_t>;
        
        nanoflann::MemoryMonitoredKDTree<
            DistanceAdaptor, 
            PointCloudAdaptor, 
            3,  // 3D points
            uint32_t> index(3, dataset, memory_threshold);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "KD-tree built successfully in " << duration.count() << " ms\n";
        std::cout << "Final memory usage: " << (index.getCurrentMemoryUsage() / (1024 * 1024)) << " MB\n\n";
        
        // Perform a test search
        std::array<float, 3> query_point = {0.0f, 0.0f, 0.0f};
        const size_t num_neighbors = 10;
        
        std::vector<uint32_t> indices(num_neighbors);
        std::vector<float> distances(num_neighbors);
        
        std::cout << "Performing nearest neighbor search...\n";
        start_time = std::chrono::high_resolution_clock::now();
        
        size_t num_found = index.knnSearch(query_point.data(), num_neighbors, 
                                         indices.data(), distances.data());
        
        end_time = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "Found " << num_found << " nearest neighbors in " << duration.count() << " ms\n";
        std::cout << "Nearest neighbors:\n";
        for (size_t i = 0; i < num_found; ++i) {
            std::cout << "  " << i + 1 << ": Point " << indices[i] 
                     << " at distance " << std::sqrt(distances[i]) << "\n";
        }
        
    } catch (const nanoflann::MemoryLimitExceededException& e) {
        std::cerr << "Memory limit exceeded: " << e.what() << "\n";
        std::cerr << "Consider reducing the dataset size or increasing the memory threshold.\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
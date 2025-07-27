/**
 * Nanoflann Memory Monitor Example
 * 
 * This example demonstrates how to use the nanoflann memory monitor
 * to track memory usage during tree building operations.
 * 
 * Features demonstrated:
 * - Setting up memory monitoring with custom thresholds
 * - Using RAII scope for tree building operations
 * - Handling memory events with callbacks
 * - Estimating memory usage for tree construction
 */

#include <memory/nanoflann_memory_monitor.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <thread>

// Simulated nanoflann tree building operation
class SimulatedNanoflannTree {
private:
    std::vector<std::vector<double>> points_;
    size_t dimension_;
    
public:
    SimulatedNanoflannTree(size_t num_points, size_t dimension) 
        : dimension_(dimension) {
        points_.reserve(num_points);
        
        // Generate random points
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dis(0.0, 1000.0);
        
        for (size_t i = 0; i < num_points; ++i) {
            std::vector<double> point(dimension);
            for (size_t j = 0; j < dimension; ++j) {
                point[j] = dis(gen);
            }
            points_.push_back(std::move(point));
        }
    }
    
    // Simulate tree building with memory allocation
    void build_tree() {
        std::cout << "Building tree with " << points_.size() 
                  << " points of dimension " << dimension_ << std::endl;
        
        // Simulate memory allocation during tree building
        std::vector<std::vector<size_t>> tree_nodes;
        std::vector<size_t> index_array;
        
        // Allocate tree nodes (simulating nanoflann's internal structure)
        tree_nodes.reserve(points_.size());
        index_array.reserve(points_.size() * 2);
        
        // Simulate the tree building process
        for (size_t i = 0; i < points_.size(); ++i) {
            // Simulate node creation
            std::vector<size_t> node_indices;
            node_indices.reserve(10); // Simulate child nodes
            
            for (size_t j = 0; j < 10 && j < points_.size(); ++j) {
                node_indices.push_back((i + j) % points_.size());
            }
            
            tree_nodes.push_back(std::move(node_indices));
            index_array.push_back(i);
            
            // Simulate some processing time
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        std::cout << "Tree building completed" << std::endl;
    }
    
    size_t get_num_points() const { return points_.size(); }
    size_t get_dimension() const { return dimension_; }
};

int main() {
    std::cout << "=== Nanoflann Memory Monitor Example ===" << std::endl;
    
    // Create a memory monitor with custom configuration
    nanoflann::MemoryMonitor::Config config;
    config.memory_threshold_mb = 50;  // 50MB threshold
    config.check_interval_ms = 50;    // Check every 50ms
    config.enable_background_monitoring = true;
    config.enable_detailed_logging = true;
    config.log_prefix = "[NANOFLANN_EXAMPLE]";
    
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
    
    // Example 1: Small tree building
    {
        std::cout << "\n--- Example 1: Small tree building ---" << std::endl;
        
        // Use RAII scope for automatic tree build tracking
        nanoflann::TreeBuildScope scope(monitor, "Small tree (10K points, 3D)");
        
        SimulatedNanoflannTree tree(10000, 3);
        
        // Estimate memory usage
        size_t estimated_memory = nanoflann::memory_utils::estimate_tree_memory_usage(
            tree.get_num_points(), tree.get_dimension(), sizeof(double));
        
        std::cout << "Estimated memory usage: " << estimated_memory << "MB" << std::endl;
        
        tree.build_tree();
        
        // Scope automatically ends here, marking tree build end
    }
    
    // Example 2: Medium tree building
    {
        std::cout << "\n--- Example 2: Medium tree building ---" << std::endl;
        
        nanoflann::TreeBuildScope scope(monitor, "Medium tree (100K points, 5D)");
        
        SimulatedNanoflannTree tree(100000, 5);
        
        size_t estimated_memory = nanoflann::memory_utils::estimate_tree_memory_usage(
            tree.get_num_points(), tree.get_dimension(), sizeof(double));
        
        std::cout << "Estimated memory usage: " << estimated_memory << "MB" << std::endl;
        
        tree.build_tree();
    }
    
    // Example 3: Large tree building (might trigger threshold)
    {
        std::cout << "\n--- Example 3: Large tree building ---" << std::endl;
        
        nanoflann::TreeBuildScope scope(monitor, "Large tree (500K points, 10D)");
        
        SimulatedNanoflannTree tree(500000, 10);
        
        size_t estimated_memory = nanoflann::memory_utils::estimate_tree_memory_usage(
            tree.get_num_points(), tree.get_dimension(), sizeof(double));
        
        std::cout << "Estimated memory usage: " << estimated_memory << "MB" << std::endl;
        
        // This might trigger memory threshold warnings
        tree.build_tree();
    }
    
    // Example 4: Manual memory checks
    {
        std::cout << "\n--- Example 4: Manual memory checks ---" << std::endl;
        
        monitor.check_memory("Before large allocation");
        
        // Simulate large memory allocation
        std::vector<std::vector<double>> large_data;
        large_data.reserve(100000);
        
        for (size_t i = 0; i < 100000; ++i) {
            large_data.emplace_back(100, 1.0); // 100 doubles per vector
        }
        
        monitor.check_memory("After large allocation");
        
        // Clear data
        large_data.clear();
        large_data.shrink_to_fit();
        
        monitor.check_memory("After clearing data");
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
        std::cout << "- " << event.context << " (" << event.memory_mb << "MB)" << std::endl;
    }
    
    std::cout << "\nExample completed successfully!" << std::endl;
    
    return 0;
}
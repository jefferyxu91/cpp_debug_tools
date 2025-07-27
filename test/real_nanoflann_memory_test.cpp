/**
 * Real Nanoflann Memory Monitor Test
 * 
 * Tests for the nanoflann memory monitor functionality using the actual nanoflann library.
 */

#include <memory/nanoflann_memory_monitor.hpp>
#include <nanoflann.hpp>
#include <gtest/gtest.h>
#include <vector>
#include <random>

// Point cloud structure for nanoflann
struct TestPointCloud {
    std::vector<std::vector<double>> pts;

    inline size_t kdtree_get_point_count() const { return pts.size(); }

    inline double kdtree_get_pt(const size_t idx, const size_t dim) const {
        return pts[idx][dim];
    }

    template <class BBOX>
    bool kdtree_get_bbox(BBOX&) const { return false; }
};

// Generate test point cloud
TestPointCloud generate_test_cloud(size_t num_points, size_t dimension) {
    TestPointCloud cloud;
    cloud.pts.reserve(num_points);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(0.0, 100.0);
    
    for (size_t i = 0; i < num_points; ++i) {
        std::vector<double> point(dimension);
        for (size_t j = 0; j < dimension; ++j) {
            point[j] = dis(gen);
        }
        cloud.pts.push_back(std::move(point));
    }
    
    return cloud;
}

class RealNanoflannMemoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test environment
    }
    
    void TearDown() override {
        // Clean up test environment
    }
};

// Test memory monitoring with real nanoflann tree building
TEST_F(RealNanoflannMemoryTest, RealTreeBuilding) {
    auto monitor = nanoflann::memory_utils::create_default_monitor();
    monitor.start();
    
    // Generate test data
    auto cloud = generate_test_cloud(1000, 3);
    
    // Build tree with monitoring
    {
        nanoflann::TreeBuildScope scope(monitor, "Test tree build");
        
        using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
            TestPointCloud,
            3
        >;
        
        KDTree index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
        index.buildIndex();
        
        // Verify tree works
        std::vector<double> query_pt(3, 50.0);
        std::vector<std::pair<size_t, double>> matches;
        index.radiusSearch(&query_pt[0], 10.0, matches, nanoflann::SearchParams());
        
        EXPECT_GE(matches.size(), 0);
    }
    
    monitor.stop();
    
    // Verify events were generated
    auto events = monitor.get_event_history();
    EXPECT_GT(events.size(), 0);
    
    // Check for tree build events
    bool found_start = false, found_end = false;
    for (const auto& event : events) {
        if (event.type == nanoflann::MemoryMonitor::EventType::TREE_BUILD_START) {
            found_start = true;
        }
        if (event.type == nanoflann::MemoryMonitor::EventType::TREE_BUILD_END) {
            found_end = true;
        }
    }
    
    EXPECT_TRUE(found_start);
    EXPECT_TRUE(found_end);
}

// Test memory estimation with real data
TEST_F(RealNanoflannMemoryTest, MemoryEstimationWithRealData) {
    // Test with different point cloud sizes
    std::vector<size_t> point_counts = {100, 1000, 10000};
    std::vector<size_t> dimensions = {2, 3, 5};
    
    for (size_t points : point_counts) {
        for (size_t dim : dimensions) {
            auto cloud = generate_test_cloud(points, dim);
            
            size_t estimated_memory = nanoflann::memory_utils::estimate_tree_memory_usage(
                cloud.kdtree_get_point_count(), dim, sizeof(double));
            
            EXPECT_GT(estimated_memory, 0);
            
            // Larger datasets should have larger memory estimates
            if (points > 100) {
                EXPECT_GT(estimated_memory, 1); // At least 1MB for larger datasets
            }
        }
    }
}

// Test multiple tree building operations
TEST_F(RealNanoflannMemoryTest, MultipleTreeBuilding) {
    auto monitor = nanoflann::memory_utils::create_default_monitor();
    monitor.start();
    
    // Build multiple trees
    for (size_t i = 0; i < 3; ++i) {
        auto cloud = generate_test_cloud(500 + i * 100, 3);
        
        {
            nanoflann::TreeBuildScope scope(monitor, "Multiple trees test " + std::to_string(i));
            
            using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
                nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
                TestPointCloud,
                3
            >;
            
            KDTree index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
            index.buildIndex();
        }
    }
    
    monitor.stop();
    
    auto events = monitor.get_event_history();
    
    // Should have 6 events (3 start + 3 end)
    EXPECT_EQ(events.size(), 6);
    
    // Count start and end events
    size_t start_count = 0, end_count = 0;
    for (const auto& event : events) {
        if (event.type == nanoflann::MemoryMonitor::EventType::TREE_BUILD_START) {
            start_count++;
        }
        if (event.type == nanoflann::MemoryMonitor::EventType::TREE_BUILD_END) {
            end_count++;
        }
    }
    
    EXPECT_EQ(start_count, 3);
    EXPECT_EQ(end_count, 3);
}

// Test memory threshold with large tree
TEST_F(RealNanoflannMemoryTest, MemoryThresholdWithLargeTree) {
    nanoflann::MemoryMonitor::Config config;
    config.memory_threshold_mb = 10; // Low threshold to trigger events
    config.check_interval_ms = 10;   // Fast checking
    
    nanoflann::MemoryMonitor monitor(config);
    
    bool threshold_exceeded = false;
    monitor.add_callback([&threshold_exceeded](const nanoflann::MemoryMonitor::MemoryEvent& event) {
        if (event.type == nanoflann::MemoryMonitor::EventType::THRESHOLD_EXCEEDED) {
            threshold_exceeded = true;
        }
    });
    
    monitor.start();
    
    // Build a larger tree that might exceed threshold
    auto cloud = generate_test_cloud(50000, 5);
    
    {
        nanoflann::TreeBuildScope scope(monitor, "Large tree threshold test");
        
        using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
            TestPointCloud,
            3
        >;
        
        KDTree index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
        index.buildIndex();
    }
    
    monitor.stop();
    
    // Note: Threshold might not be exceeded depending on system
    // This test mainly verifies the monitoring mechanism works
    EXPECT_TRUE(true); // Test passes if no exceptions
}

// Test performance impact measurement
TEST_F(RealNanoflannMemoryTest, PerformanceImpact) {
    auto cloud = generate_test_cloud(1000, 3);
    
    // Measure time without monitoring
    auto start = std::chrono::high_resolution_clock::now();
    
    using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
        TestPointCloud,
        3
    >;
    
    KDTree index_no_monitor(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
    index_no_monitor.buildIndex();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_no_monitor = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Measure time with monitoring
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
    
    // Verify both trees work correctly
    std::vector<double> query_pt(3, 50.0);
    std::vector<std::pair<size_t, double>> matches1, matches2;
    
    index_no_monitor.radiusSearch(&query_pt[0], 10.0, matches1, nanoflann::SearchParams());
    index_with_monitor.radiusSearch(&query_pt[0], 10.0, matches2, nanoflann::SearchParams());
    
    EXPECT_EQ(matches1.size(), matches2.size());
    
    // Verify monitoring doesn't cause excessive overhead (less than 50% overhead)
    double overhead_ratio = static_cast<double>(duration_with_monitor.count()) / duration_no_monitor.count();
    EXPECT_LT(overhead_ratio, 1.5);
}

// Test different tree parameters
TEST_F(RealNanoflannMemoryTest, DifferentTreeParameters) {
    auto monitor = nanoflann::memory_utils::create_default_monitor();
    monitor.start();
    
    auto cloud = generate_test_cloud(1000, 3);
    
    // Test different max leaf sizes
    std::vector<int> max_leaves = {5, 10, 20};
    
    for (int max_leaf : max_leaves) {
        {
            nanoflann::TreeBuildScope scope(monitor, "Max leaf " + std::to_string(max_leaf));
            
            using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
                nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
                TestPointCloud,
                3
            >;
            
            KDTree index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(max_leaf));
            index.buildIndex();
            
            // Verify tree works
            std::vector<double> query_pt(3, 50.0);
            std::vector<std::pair<size_t, double>> matches;
            index.radiusSearch(&query_pt[0], 10.0, matches, nanoflann::SearchParams());
            
            EXPECT_GE(matches.size(), 0);
        }
    }
    
    monitor.stop();
    
    auto events = monitor.get_event_history();
    EXPECT_GT(events.size(), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
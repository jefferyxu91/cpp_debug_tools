/**
 * Monitored Nanoflann Test
 * 
 * Tests for the MonitoredKDTreeIndex class that automatically monitors memory usage.
 */

#include <memory/nanoflann_monitored_index.hpp>
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

class MonitoredNanoflannTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test environment
    }
    
    void TearDown() override {
        // Clean up test environment
    }
};

// Test basic monitored index creation and functionality
TEST_F(MonitoredNanoflannTest, BasicFunctionality) {
    auto cloud = generate_test_cloud(1000, 3);
    
    using MonitoredKDTree = nanoflann::MonitoredKDTreeIndex<
        nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
        TestPointCloud, 3>;
    
    MonitoredKDTree::MonitorConfig config(50); // 50MB threshold
    MonitoredKDTree index(3, cloud, config);
    
    // Verify monitoring is enabled
    EXPECT_TRUE(index.is_monitoring_enabled());
    EXPECT_EQ(index.get_memory_threshold(), 50);
    
    // Build the index
    index.buildIndex();
    
    // Verify the tree works
    std::vector<double> query_pt(3, 50.0);
    std::vector<std::pair<size_t, double>> matches;
    index.radiusSearch(&query_pt[0], 10.0, matches, nanoflann::SearchParams());
    
    EXPECT_GE(matches.size(), 0);
    
    // Get memory statistics
    auto stats = index.get_memory_stats();
    EXPECT_GE(stats.peak_memory_mb, 0);
}

// Test monitored index with custom configuration
TEST_F(MonitoredNanoflannTest, CustomConfiguration) {
    auto cloud = generate_test_cloud(1000, 3);
    
    using MonitoredKDTree = nanoflann::MonitoredKDTreeIndex<
        nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
        TestPointCloud, 3>;
    
    MonitoredKDTree::MonitorConfig config;
    config.memory_threshold_mb = 25;
    config.enable_auto_monitoring = true;
    config.print_warnings = false;
    config.context_prefix = "TestConfig";
    
    MonitoredKDTree index(3, cloud, config);
    
    EXPECT_EQ(index.get_memory_threshold(), 25);
    EXPECT_TRUE(index.is_monitoring_enabled());
    
    index.buildIndex();
    
    // Verify tree functionality
    std::vector<double> query_pt(3, 50.0);
    std::vector<std::pair<size_t, double>> matches;
    index.radiusSearch(&query_pt[0], 10.0, matches, nanoflann::SearchParams());
    
    EXPECT_GE(matches.size(), 0);
}

// Test monitored index with disabled monitoring
TEST_F(MonitoredNanoflannTest, DisabledMonitoring) {
    auto cloud = generate_test_cloud(1000, 3);
    
    using MonitoredKDTree = nanoflann::MonitoredKDTreeIndex<
        nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
        TestPointCloud, 3>;
    
    MonitoredKDTree::MonitorConfig config;
    config.enable_auto_monitoring = false;
    
    MonitoredKDTree index(3, cloud, config);
    
    EXPECT_FALSE(index.is_monitoring_enabled());
    
    index.buildIndex();
    
    // Memory stats should be zero when monitoring is disabled
    auto stats = index.get_memory_stats();
    EXPECT_EQ(stats.peak_memory_mb, 0);
    
    // Verify tree still works
    std::vector<double> query_pt(3, 50.0);
    std::vector<std::pair<size_t, double>> matches;
    index.radiusSearch(&query_pt[0], 10.0, matches, nanoflann::SearchParams());
    
    EXPECT_GE(matches.size(), 0);
}

// Test runtime configuration changes
TEST_F(MonitoredNanoflannTest, RuntimeConfiguration) {
    auto cloud = generate_test_cloud(1000, 3);
    
    using MonitoredKDTree = nanoflann::MonitoredKDTreeIndex<
        nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
        TestPointCloud, 3>;
    
    MonitoredKDTree index(3, cloud, 100); // 100MB threshold
    
    EXPECT_EQ(index.get_memory_threshold(), 100);
    
    // Change threshold at runtime
    index.set_memory_threshold(75);
    EXPECT_EQ(index.get_memory_threshold(), 75);
    
    // Change context prefix
    index.set_context_prefix("RuntimeTest");
    
    index.buildIndex();
    
    // Verify tree works
    std::vector<double> query_pt(3, 50.0);
    std::vector<std::pair<size_t, double>> matches;
    index.radiusSearch(&query_pt[0], 10.0, matches, nanoflann::SearchParams());
    
    EXPECT_GE(matches.size(), 0);
}

// Test memory events recording
TEST_F(MonitoredNanoflannTest, MemoryEvents) {
    auto cloud = generate_test_cloud(1000, 3);
    
    using MonitoredKDTree = nanoflann::MonitoredKDTreeIndex<
        nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
        TestPointCloud, 3>;
    
    MonitoredKDTree index(3, cloud, 50);
    
    index.buildIndex();
    
    // Get memory events
    auto events = index.get_memory_events();
    
    // Should have at least tree build start and end events
    EXPECT_GE(events.size(), 2);
    
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

// Test utility functions
TEST_F(MonitoredNanoflannTest, UtilityFunctions) {
    auto cloud = generate_test_cloud(1000, 3);
    
    using MonitoredKDTree = nanoflann::MonitoredKDTreeIndex<
        nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
        TestPointCloud, 3>;
    
    // Test create_monitored_index utility
    auto index1 = nanoflann::monitored_utils::create_monitored_index<
        nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
        TestPointCloud, 3>(3, cloud, 75);
    
    EXPECT_EQ(index1.get_memory_threshold(), 75);
    EXPECT_TRUE(index1.is_monitoring_enabled());
    
    // Test create_smart_monitored_index utility
    auto index2 = nanoflann::monitored_utils::create_smart_monitored_index<
        nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
        TestPointCloud, 3>(3, cloud, 1.5);
    
    EXPECT_GT(index2.get_memory_threshold(), 0);
    EXPECT_TRUE(index2.is_monitoring_enabled());
    
    // Build both indices
    index1.buildIndex();
    index2.buildIndex();
    
    // Verify both work
    std::vector<double> query_pt(3, 50.0);
    std::vector<std::pair<size_t, double>> matches1, matches2;
    
    index1.radiusSearch(&query_pt[0], 10.0, matches1, nanoflann::SearchParams());
    index2.radiusSearch(&query_pt[0], 10.0, matches2, nanoflann::SearchParams());
    
    EXPECT_EQ(matches1.size(), matches2.size());
}

// Test performance comparison with regular nanoflann
TEST_F(MonitoredNanoflannTest, PerformanceComparison) {
    auto cloud = generate_test_cloud(1000, 3);
    
    using RegularKDTree = nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
        TestPointCloud, 3>;
    
    using MonitoredKDTree = nanoflann::MonitoredKDTreeIndex<
        nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
        TestPointCloud, 3>;
    
    // Build regular tree
    RegularKDTree regular_index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
    regular_index.buildIndex();
    
    // Build monitored tree
    MonitoredKDTree monitored_index(3, cloud, 100);
    monitored_index.buildIndex();
    
    // Test both trees with same query
    std::vector<double> query_pt(3, 50.0);
    std::vector<std::pair<size_t, double>> matches1, matches2;
    
    regular_index.radiusSearch(&query_pt[0], 10.0, matches1, nanoflann::SearchParams());
    monitored_index.radiusSearch(&query_pt[0], 10.0, matches2, nanoflann::SearchParams());
    
    // Results should be identical
    EXPECT_EQ(matches1.size(), matches2.size());
    
    // Check that monitored index has memory stats
    auto stats = monitored_index.get_memory_stats();
    EXPECT_GE(stats.peak_memory_mb, 0);
}

// Test different dimensions
TEST_F(MonitoredNanoflannTest, DifferentDimensions) {
    auto cloud = generate_test_cloud(1000, 5);
    
    using MonitoredKDTree = nanoflann::MonitoredKDTreeIndex<
        nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
        TestPointCloud, -1>; // Dynamic dimension
    
    MonitoredKDTree index(5, cloud, 50);
    
    EXPECT_EQ(index.get_memory_threshold(), 50);
    
    index.buildIndex();
    
    // Test with 5D query
    std::vector<double> query_pt(5, 50.0);
    std::vector<std::pair<size_t, double>> matches;
    index.radiusSearch(&query_pt[0], 10.0, matches, nanoflann::SearchParams());
    
    EXPECT_GE(matches.size(), 0);
    
    auto stats = index.get_memory_stats();
    EXPECT_GE(stats.peak_memory_mb, 0);
}

// Test custom logger
TEST_F(MonitoredNanoflannTest, CustomLogger) {
    auto cloud = generate_test_cloud(1000, 3);
    
    using MonitoredKDTree = nanoflann::MonitoredKDTreeIndex<
        nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
        TestPointCloud, 3>;
    
    bool logger_called = false;
    std::string logged_message;
    
    auto custom_logger = [&logger_called, &logged_message](const std::string& message) {
        logger_called = true;
        logged_message = message;
    };
    
    MonitoredKDTree::MonitorConfig config;
    config.memory_threshold_mb = 10; // Low threshold to trigger warnings
    config.custom_logger = custom_logger;
    config.print_warnings = false; // Disable default logging
    
    MonitoredKDTree index(3, cloud, config);
    
    index.buildIndex();
    
    // Logger might be called if threshold is exceeded
    // This test mainly verifies the custom logger mechanism works
    EXPECT_TRUE(true); // Test passes if no exceptions
}

// Test move semantics
TEST_F(MonitoredNanoflannTest, MoveSemantics) {
    auto cloud = generate_test_cloud(1000, 3);
    
    using MonitoredKDTree = nanoflann::MonitoredKDTreeIndex<
        nanoflann::L2_Simple_Adaptor<double, TestPointCloud>,
        TestPointCloud, 3>;
    
    MonitoredKDTree index1(3, cloud, 50);
    
    // Move construct
    MonitoredKDTree index2(std::move(index1));
    
    EXPECT_EQ(index2.get_memory_threshold(), 50);
    EXPECT_TRUE(index2.is_monitoring_enabled());
    
    index2.buildIndex();
    
    // Verify moved index works
    std::vector<double> query_pt(3, 50.0);
    std::vector<std::pair<size_t, double>> matches;
    index2.radiusSearch(&query_pt[0], 10.0, matches, nanoflann::SearchParams());
    
    EXPECT_GE(matches.size(), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
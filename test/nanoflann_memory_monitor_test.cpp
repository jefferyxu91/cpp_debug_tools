#include <gtest/gtest.h>
#include <vector>
#include <array>
#include <random>
#include <memory>
#include <stdexcept>

// Include the nanoflann memory monitor
#include "../include/memory/nanoflann_debug/nanoflann_memory_monitor.hpp"

// Test fixture for nanoflann memory monitor tests
class NanoflannMemoryMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Generate test data
        generateTestData();
    }

    void generateTestData() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-100.0f, 100.0f);

        test_points_.clear();
        test_points_.reserve(1000);
        
        for (size_t i = 0; i < 1000; ++i) {
            test_points_.push_back({dis(gen), dis(gen), dis(gen)});
        }
    }

    // Dataset adaptor for testing
    struct TestDatasetAdaptor {
        const std::vector<std::array<float, 3>>& points;
        
        explicit TestDatasetAdaptor(const std::vector<std::array<float, 3>>& pts) 
            : points(pts) {}
        
        inline size_t kdtree_get_point_count() const { return points.size(); }
        inline float kdtree_get_pt(const size_t idx, const size_t dim) const {
            return points[idx][dim];
        }
        template <class BBOX>
        bool kdtree_get_bbox(BBOX& /*bb*/) const { return false; }
    };

    std::vector<std::array<float, 3>> test_points_;
};

// Test basic memory monitor functionality
TEST_F(NanoflannMemoryMonitorTest, BasicMemoryMonitor) {
    TestDatasetAdaptor dataset(test_points_);
    
    // Test with a reasonable memory threshold
    const size_t memory_threshold = 100 * 1024 * 1024; // 100 MB
    
    // Create memory-monitored KD-tree
    nanoflann::MemoryMonitoredKDTree<
        nanoflann::L2_Simple_Adaptor<float, TestDatasetAdaptor, float, uint32_t>,
        TestDatasetAdaptor,
        3,
        uint32_t> index(3, dataset, memory_threshold);
    
    // Test that the index was built successfully by checking point count
    EXPECT_EQ(dataset.kdtree_get_point_count(), test_points_.size());
}

// Test memory limit exceeded exception
TEST_F(NanoflannMemoryMonitorTest, MemoryLimitExceeded) {
    TestDatasetAdaptor dataset(test_points_);
    
    // Set a very low memory threshold to trigger the exception
    const size_t memory_threshold = 1024; // 1 KB
    
    bool exception_thrown = false;
    try {
        nanoflann::MemoryMonitoredKDTree<
            nanoflann::L2_Simple_Adaptor<float, TestDatasetAdaptor, float, uint32_t>,
            TestDatasetAdaptor,
            3,
            uint32_t> index(3, dataset, memory_threshold);
    } catch (const nanoflann::MemoryLimitExceededException& e) {
        exception_thrown = true;
    }
    
    EXPECT_TRUE(exception_thrown);
}

// Test search functionality after successful build
TEST_F(NanoflannMemoryMonitorTest, SearchFunctionality) {
    TestDatasetAdaptor dataset(test_points_);
    const size_t memory_threshold = 100 * 1024 * 1024; // 100 MB
    
    nanoflann::MemoryMonitoredKDTree<
        nanoflann::L2_Simple_Adaptor<float, TestDatasetAdaptor, float, uint32_t>,
        TestDatasetAdaptor,
        3,
        uint32_t> index(3, dataset, memory_threshold);
    
    // Test KNN search
    std::array<float, 3> query_point = {0.0f, 0.0f, 0.0f};
    std::vector<uint32_t> indices(5);
    std::vector<float> distances(5);
    
    size_t num_found = index.knnSearch(query_point.data(), 5, indices.data(), distances.data());
    
    EXPECT_GT(num_found, 0);
    EXPECT_LE(num_found, 5);
}

// Test memory monitor with custom parameters
TEST_F(NanoflannMemoryMonitorTest, CustomParameters) {
    TestDatasetAdaptor dataset(test_points_);
    const size_t memory_threshold = 100 * 1024 * 1024; // 100 MB
    
    nanoflann::KDTreeSingleIndexAdaptorParams params;
    params.leaf_max_size = 20;
    params.n_thread_build = 1;
    
    nanoflann::MemoryMonitoredKDTree<
        nanoflann::L2_Simple_Adaptor<float, TestDatasetAdaptor, float, uint32_t>,
        TestDatasetAdaptor,
        3,
        uint32_t> index(3, dataset, params, memory_threshold);
    
    EXPECT_EQ(dataset.kdtree_get_point_count(), test_points_.size());
}

// Test memory monitor exception message
TEST_F(NanoflannMemoryMonitorTest, ExceptionMessage) {
    TestDatasetAdaptor dataset(test_points_);
    const size_t memory_threshold = 1024; // 1 KB
    
    try {
        nanoflann::MemoryMonitoredKDTree<
            nanoflann::L2_Simple_Adaptor<float, TestDatasetAdaptor, float, uint32_t>,
            TestDatasetAdaptor,
            3,
            uint32_t> index(3, dataset, memory_threshold);
        FAIL() << "Expected MemoryLimitExceededException";
    } catch (const nanoflann::MemoryLimitExceededException& e) {
        std::string message = e.what();
        EXPECT_FALSE(message.empty());
        EXPECT_NE(message.find("Memory limit exceeded"), std::string::npos);
        EXPECT_NE(message.find("bytes"), std::string::npos);
    }
}

// Test memory monitor performance (should not be significantly slower)
TEST_F(NanoflannMemoryMonitorTest, PerformanceTest) {
    TestDatasetAdaptor dataset(test_points_);
    const size_t memory_threshold = 100 * 1024 * 1024; // 100 MB
    
    auto start = std::chrono::high_resolution_clock::now();
    
    nanoflann::MemoryMonitoredKDTree<
        nanoflann::L2_Simple_Adaptor<float, TestDatasetAdaptor, float, uint32_t>,
        TestDatasetAdaptor,
        3,
        uint32_t> index(3, dataset, memory_threshold);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Building should complete in reasonable time (less than 1 second for 1000 points)
    EXPECT_LT(duration.count(), 1000);
}

// Test memory monitor with empty dataset
TEST_F(NanoflannMemoryMonitorTest, EmptyDataset) {
    std::vector<std::array<float, 3>> empty_points;
    TestDatasetAdaptor dataset(empty_points);
    const size_t memory_threshold = 10 * 1024 * 1024; // 10 MB
    
    // Empty dataset should throw an exception
    bool exception_thrown = false;
    try {
        nanoflann::MemoryMonitoredKDTree<
            nanoflann::L2_Simple_Adaptor<float, TestDatasetAdaptor, float, uint32_t>,
            TestDatasetAdaptor,
            3,
            uint32_t> index(3, dataset, memory_threshold);
    } catch (const std::exception& e) {
        exception_thrown = true;
    }
    
    EXPECT_TRUE(exception_thrown);
    EXPECT_EQ(dataset.kdtree_get_point_count(), 0);
}

// Test memory monitor with single point
TEST_F(NanoflannMemoryMonitorTest, SinglePoint) {
    std::vector<std::array<float, 3>> single_point = {{1.0f, 2.0f, 3.0f}};
    TestDatasetAdaptor dataset(single_point);
    const size_t memory_threshold = 10 * 1024 * 1024; // 10 MB
    
    nanoflann::MemoryMonitoredKDTree<
        nanoflann::L2_Simple_Adaptor<float, TestDatasetAdaptor, float, uint32_t>,
        TestDatasetAdaptor,
        3,
        uint32_t> index(3, dataset, memory_threshold);
    
    EXPECT_EQ(dataset.kdtree_get_point_count(), 1);
    
    // Test search on single point
    std::array<float, 3> query = {0.0f, 0.0f, 0.0f};
    std::vector<uint32_t> indices(1);
    std::vector<float> distances(1);
    
    size_t num_found = index.knnSearch(query.data(), 1, indices.data(), distances.data());
    EXPECT_EQ(num_found, 1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
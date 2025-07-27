#include <gtest/gtest.h>
#include <memory/nanoflann_memory_watcher.hpp>
#include <atomic>
#include <vector>
#include <thread>
#include <chrono>
#include <nanoflann.hpp>

// Helper point cloud for nanoflann tests
struct PointCloud {
    struct Point { double x,y,z; };
    std::vector<Point> pts;
    // nanoflann interface
    inline size_t kdtree_get_point_count() const { return pts.size(); }
    inline double kdtree_get_pt(const size_t idx, const size_t dim) const {
        if(dim==0) return pts[idx].x;
        if(dim==1) return pts[idx].y;
        return pts[idx].z;
    }
    template<class BBOX> bool kdtree_get_bbox(BBOX&) const { return false; }
};

TEST(PeakMemoryWatcherTest, CallbackTriggeredWhenExceedsThreshold) {
    constexpr std::size_t THRESH = 1 * 1024 * 1024; // 1 MB
    std::atomic<bool> called{false};

    MemoryWatch::watchPeakMemory(
        THRESH,
        [&](){
            std::vector<char> buf(5 * 1024 * 1024); // 5 MB allocation
            (void)buf.data(); // Suppress unused warning
        },
        std::chrono::milliseconds(5),
        [&](std::size_t){ called = true; }
    );

    EXPECT_TRUE(called.load());
}

TEST(PeakMemoryWatcherTest, CallbackNotTriggeredWhenBelowThreshold) {
    constexpr std::size_t THRESH = 200 * 1024 * 1024; // 200 MB, intentionally high
    std::atomic<bool> called{false};

    MemoryWatch::watchPeakMemory(
        THRESH,
        [&](){
            std::vector<char> buf(1 * 1024 * 1024); // 1 MB allocation
            (void)buf.data();
        },
        std::chrono::milliseconds(5),
        [&](std::size_t){ called = true; }
    );

    EXPECT_FALSE(called.load());
}

TEST(NanoflannWatcherTest, CallbackTriggeredOnKDTreeBuild) {
    // Generate a small but non-trivial point cloud
    PointCloud cloud;
    constexpr size_t NPTS = 100000; // 100k points ~ 2.4 MB raw data
    cloud.pts.resize(NPTS);
    for(size_t i=0;i<NPTS;++i) {
        cloud.pts[i].x = static_cast<double>(i%1000);
        cloud.pts[i].y = static_cast<double>((i/1000)%1000);
        cloud.pts[i].z = static_cast<double>(i);
    }

    using namespace nanoflann;
    using Metric   = L2_Simple_Adaptor<double, PointCloud>;
    using KDTree   = KDTreeSingleIndexAdaptor<Metric, PointCloud, 3>;

    KDTree index(3, cloud, KDTreeSingleIndexAdaptorParams(10));

    std::atomic<bool> called{false};
    constexpr std::size_t THRESH = 1 * 1024 * 1024; // 1 MB

    MemoryWatch::watchNanoflannBuild(index, THRESH, 0,
        [&](std::size_t){ called = true; },
        std::chrono::milliseconds(5));

    EXPECT_TRUE(called.load());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
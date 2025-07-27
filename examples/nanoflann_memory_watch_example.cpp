/**
 * nanoflann memory watch example
 * ---------------------------------
 * Demonstrates how to use MemoryWatch::PeakMemoryWatcher (or the helper
 * function watchPeakMemory) to detect large memory spikes – typical when a
 * nanoflann index is being built.
 *
 * This example purposely allocates a large std::vector to trigger the warning.
 * Replace the dummy allocation with your actual nanoflann index build.
 */

#include <nanoflann.hpp>
#include <memory/nanoflann_memory_watcher.hpp>
#include <vector>
#include <iostream>
#include <cstdlib>

// ----- PointCloud adaptor for nanoflann ----
struct PointCloud {
    struct Point { double x, y, z; };
    std::vector<Point> pts;

    // Must return the number of data points
    inline size_t kdtree_get_point_count() const { return pts.size(); }

    // Returns the dim'th component of the idx'th point in the class:
    inline double kdtree_get_pt(const size_t idx, const size_t dim) const {
        if (dim == 0) return pts[idx].x;
        else if (dim == 1) return pts[idx].y;
        else return pts[idx].z;
    }

    // Optional bounding-box computation
    template <class BBOX>
    bool kdtree_get_bbox(BBOX&) const { return false; }
};

int main() {
    constexpr std::size_t THRESHOLD = 50 * 1024 * 1024; // 50 MB

    std::cout << "Watching memory usage with threshold " << (THRESHOLD / (1024*1024)) << " MB..." << std::endl;

    PointCloud cloud;

    // Generate many points to make index memory heavy
    constexpr size_t NPTS = 3'000'000; // 3M points
    cloud.pts.resize(NPTS);
    for(size_t i=0;i<NPTS;++i) {
        cloud.pts[i].x = drand48();
        cloud.pts[i].y = drand48();
        cloud.pts[i].z = drand48();
    }

    using namespace nanoflann;
    using KDTree = KDTreeSingleIndexAdaptor<
        L2_Simple_Adaptor<double, PointCloud>, PointCloud, 3 /*dim*/>;

    KDTree index(3, cloud, KDTreeSingleIndexAdaptorParams(10));

    // Watch both RSS and pool memory (pool threshold 30MB)
    MemoryWatch::watchNanoflannBuild(index, THRESHOLD, 30*1024*1024);
    
    std::cout << "Finished without crash." << std::endl;
    return 0;
}
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

#include <memory/nanoflann_memory_watcher.hpp>
#include <vector>
#include <iostream>

int main() {
    constexpr std::size_t THRESHOLD = 50 * 1024 * 1024; // 50 MB

    std::cout << "Watching memory usage with threshold " << (THRESHOLD / (1024*1024)) << " MB..." << std::endl;

    MemoryWatch::watchPeakMemory(THRESHOLD, [](){
        // Simulate heavy memory usage (~160 MB)
        std::vector<double> bigVec;
        bigVec.resize(20'000'000); // 20M doubles ≈ 160 MB
    });

    std::cout << "Finished without crash." << std::endl;
    return 0;
}
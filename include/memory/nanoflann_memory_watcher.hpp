#ifndef NANOFLANN_MEMORY_WATCHER_HPP
#define NANOFLANN_MEMORY_WATCHER_HPP

#include <atomic>
#include <chrono>
#include <csignal>
#include <functional>
#include <fstream>
#include <iostream>
#include <thread>
#include <unistd.h>  // for sysconf

namespace MemoryWatch {

/**
 * PeakMemoryWatcher
 * -----------------
 * Lightweight, header-only helper to keep track of the peak Resident Set Size (RSS)
 * of the current process during a scoped period (e.g. while a nanoflann index is
 * being built). If the peak memory usage minus the baseline exceeds a user-given
 * threshold, a user-provided callback is executed (defaults to writing a message
 * to std::cerr).
 *
 * This class is intended to be *non-intrusive* and *low-overhead*:
 *   1. No modifications of nanoflann or allocation hooks are required.
 *   2. Sampling is done in a dedicated background thread with a configurable
 *      period (default 10 ms). The thread sleeps most of the time, so the
 *      overhead is negligible even for long-running index builds.
 *   3. Works on Linux by reading /proc/self/statm. (If /proc is not available
 *      the watcher is silently disabled so it never fails user code.)
 *
 * Typical usage:
 *
 *   {
 *       const std::size_t THRESH = 200 * 1024 * 1024; // 200 MB
 *       MemoryWatch::PeakMemoryWatcher watcher(THRESH);
 *       index.buildIndex();  // your nanoflann call
 *       // watcher automatically stops & reports in its destructor.
 *   }
 */
class PeakMemoryWatcher {
public:
    using Callback = std::function<void(std::size_t)>;

    explicit PeakMemoryWatcher(std::size_t threshold_bytes,
                               std::chrono::milliseconds sampling_period = std::chrono::milliseconds(10),
                               Callback callback = defaultCallback)
        : threshold_(threshold_bytes),
          period_(sampling_period),
          callback_(std::move(callback)),
          baseline_rss_(currentRSS()),
          peak_rss_(baseline_rss_),
          running_(true)
    {
        // If currentRSS() returns 0, /proc is probably unavailable – disable.
        if (baseline_rss_ == 0) {
            running_ = false;
            return;
        }
        watcher_thread_ = std::thread([this]() { this->watchLoop(); });
    }

    // Non-copyable / non-movable: watcher owns a thread.
    PeakMemoryWatcher(const PeakMemoryWatcher&) = delete;
    PeakMemoryWatcher& operator=(const PeakMemoryWatcher&) = delete;

    ~PeakMemoryWatcher() {
        if (running_) {
            running_.store(false, std::memory_order_relaxed);
            if (watcher_thread_.joinable())
                watcher_thread_.join();
        }
        // Capture RSS one last time, in case we missed the final spike while shutting down.
        const std::size_t final_rss = currentRSS();
        if (final_rss > peak_rss_) {
            peak_rss_ = final_rss;
        }
        reportIfExceeded();
    }

private:
    static void defaultCallback(std::size_t bytes) {
        std::cerr << "[MemoryWatch] Peak RSS exceeded threshold by "
                  << (bytes / (1024.0 * 1024.0)) << " MB (" << bytes << " bytes)."
                  << std::endl;
    }

    void watchLoop() {
        while (running_.load(std::memory_order_relaxed)) {
            const std::size_t rss = currentRSS();
            if (rss > peak_rss_) {
                peak_rss_ = rss;
            }
            std::this_thread::sleep_for(period_);
        }
    }

    void reportIfExceeded() const {
        const std::size_t diff = peak_rss_ - baseline_rss_;
        if (diff > threshold_) {
            callback_(diff);
        }
    }

    // Read the current Resident Set Size (RSS) in bytes. Linux-specific.
    static std::size_t currentRSS() {
        std::ifstream statm("/proc/self/statm");
        if (!statm.is_open()) {
            return 0; // /proc unavailable – return 0 so watcher disables itself.
        }
        long pages_resident = 0;
        long unused = 0;
        statm >> unused >> pages_resident;
        const long page_size = sysconf(_SC_PAGESIZE);
        return static_cast<std::size_t>(pages_resident) * static_cast<std::size_t>(page_size);
    }

    // Data members
    const std::size_t threshold_;
    const std::chrono::milliseconds period_;
    const Callback callback_;

    const std::size_t baseline_rss_;
    std::size_t peak_rss_;

    std::atomic<bool> running_;
    std::thread watcher_thread_;
};

// Convenience RAII wrapper to watch memory just for the duration of a callable.
// Example:
//   watchPeakMemory(THRESH, [&]{ index.buildIndex(); });
// This helper spares you from explicitly declaring a PeakMemoryWatcher variable.

template<typename F>
void watchPeakMemory(std::size_t threshold_bytes,
                     F&& func,
                     std::chrono::milliseconds sampling_period = std::chrono::milliseconds(10),
                     typename PeakMemoryWatcher::Callback cb = nullptr)
{
    if (cb) {
        PeakMemoryWatcher watcher(threshold_bytes, sampling_period, std::move(cb));
        func();
    } else {
        PeakMemoryWatcher watcher(threshold_bytes, sampling_period);
        func();
    }
    // watcher destroyed here, reports automatically
}

} // namespace MemoryWatch

#endif // NANOFLANN_MEMORY_WATCHER_HPP
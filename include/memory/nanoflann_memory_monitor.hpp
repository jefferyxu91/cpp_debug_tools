#ifndef NANOFLANN_MEMORY_MONITOR_HPP
#define NANOFLANN_MEMORY_MONITOR_HPP

#include <chrono>
#include <iostream>
#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unordered_map>
#include <mutex>

namespace NanoFlannMemory {

// Forward declarations
class MemoryMonitor;
class MemoryTracker;

// Configuration structure
struct MonitorConfig {
    size_t threshold_bytes = 50 * 1024 * 1024;  // 50MB default threshold
    std::chrono::milliseconds sampling_interval{100};  // Sample every 100ms
    bool enable_detailed_tracking = false;  // Detailed per-allocation tracking
    bool enable_periodic_reports = true;   // Enable periodic memory reports
    bool enable_threshold_alerts = true;   // Enable threshold-based alerts
    std::string log_file_path = "";        // Empty means stderr only
    
    // Callback functions
    std::function<void(size_t, const std::string&)> threshold_callback = nullptr;
    std::function<void(size_t, size_t, size_t)> periodic_callback = nullptr;  // current, peak, allocated_count
};

// Thread-safe memory statistics
struct MemoryStats {
    std::atomic<size_t> current_usage{0};
    std::atomic<size_t> peak_usage{0};
    std::atomic<size_t> total_allocations{0};
    std::atomic<size_t> allocation_count{0};
    std::atomic<bool> threshold_exceeded{false};
    
    void reset() {
        current_usage = 0;
        peak_usage = 0;
        total_allocations = 0;
        allocation_count = 0;
        threshold_exceeded = false;
    }
};

// Memory allocation tracker with minimal overhead
class MemoryTracker {
private:
    static MemoryStats stats_;
    static MonitorConfig config_;
    static std::mutex config_mutex_;
    static std::atomic<bool> monitoring_enabled_;
    
    // Track allocations for detailed mode
    static std::unordered_map<void*, size_t> allocations_;
    static std::mutex allocations_mutex_;
    
public:
    // Enable/disable monitoring
    static void enable_monitoring(bool enable = true) {
        monitoring_enabled_.store(enable);
    }
    
    static bool is_monitoring_enabled() {
        return monitoring_enabled_.load();
    }
    
    // Configuration
    static void configure(const MonitorConfig& config) {
        std::lock_guard<std::mutex> lock(config_mutex_);
        config_ = config;
    }
    
    static MonitorConfig get_config() {
        std::lock_guard<std::mutex> lock(config_mutex_);
        return config_;
    }
    
    // Track allocation
    static void track_allocation(void* ptr, size_t size) {
        if (!monitoring_enabled_.load()) return;
        
        size_t new_current = stats_.current_usage.fetch_add(size) + size;
        stats_.allocation_count.fetch_add(1);
        stats_.total_allocations.fetch_add(size);
        
        // Update peak usage
        size_t current_peak = stats_.peak_usage.load();
        while (new_current > current_peak && 
               !stats_.peak_usage.compare_exchange_weak(current_peak, new_current)) {
            // Retry if another thread updated peak_usage
        }
        
        // Detailed tracking if enabled
        if (config_.enable_detailed_tracking) {
            std::lock_guard<std::mutex> lock(allocations_mutex_);
            allocations_[ptr] = size;
        }
        
        // Check threshold
        if (config_.enable_threshold_alerts && new_current > config_.threshold_bytes) {
            if (!stats_.threshold_exceeded.exchange(true)) {
                // First time exceeding threshold
                trigger_threshold_alert(new_current);
            }
        }
    }
    
    // Track deallocation
    static void track_deallocation(void* ptr) {
        if (!monitoring_enabled_.load() || !ptr) return;
        
        size_t size = 0;
        
        if (config_.enable_detailed_tracking) {
            std::lock_guard<std::mutex> lock(allocations_mutex_);
            auto it = allocations_.find(ptr);
            if (it != allocations_.end()) {
                size = it->second;
                allocations_.erase(it);
            }
        } else {
            // For non-detailed mode, we can't track individual deallocations precisely
            // This is a limitation of the lightweight approach
            return;
        }
        
        if (size > 0) {
            stats_.current_usage.fetch_sub(size);
            if (stats_.current_usage.load() < config_.threshold_bytes) {
                stats_.threshold_exceeded.store(false);
            }
        }
    }
    
    // Get current statistics
    static MemoryStats get_stats() {
        return stats_;
    }
    
    // Reset statistics
    static void reset_stats() {
        stats_.reset();
        if (config_.enable_detailed_tracking) {
            std::lock_guard<std::mutex> lock(allocations_mutex_);
            allocations_.clear();
        }
    }
    
private:
    static void trigger_threshold_alert(size_t current_usage) {
        auto config = get_config();
        
        std::string message = "[NANOFLANN MEMORY ALERT] Memory usage exceeded threshold: " +
                            std::to_string(current_usage / (1024.0 * 1024.0)) + " MB " +
                            "(threshold: " + std::to_string(config.threshold_bytes / (1024.0 * 1024.0)) + " MB)";
        
        // Log to file if specified
        if (!config.log_file_path.empty()) {
            std::ofstream log_file(config.log_file_path, std::ios::app);
            if (log_file.is_open()) {
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                log_file << "[" << std::ctime(&time_t) << "] " << message << std::endl;
            }
        }
        
        // Output to stderr
        std::cerr << message << std::endl;
        
        // Call custom callback if provided
        if (config.threshold_callback) {
            config.threshold_callback(current_usage, message);
        }
    }
};

// Static member definitions
MemoryStats MemoryTracker::stats_;
MonitorConfig MemoryTracker::config_;
std::mutex MemoryTracker::config_mutex_;
std::atomic<bool> MemoryTracker::monitoring_enabled_{false};
std::unordered_map<void*, size_t> MemoryTracker::allocations_;
std::mutex MemoryTracker::allocations_mutex_;

// Periodic memory monitor that runs in a separate thread
class PeriodicMonitor {
private:
    std::thread monitor_thread_;
    std::atomic<bool> running_{false};
    
public:
    void start() {
        if (running_.load()) return;
        
        running_.store(true);
        monitor_thread_ = std::thread([this]() {
            while (running_.load()) {
                auto config = MemoryTracker::get_config();
                if (config.enable_periodic_reports) {
                    auto stats = MemoryTracker::get_stats();
                    
                    if (config.periodic_callback) {
                        config.periodic_callback(
                            stats.current_usage.load(),
                            stats.peak_usage.load(),
                            stats.allocation_count.load()
                        );
                    }
                }
                
                std::this_thread::sleep_for(MemoryTracker::get_config().sampling_interval);
            }
        });
    }
    
    void stop() {
        if (!running_.load()) return;
        
        running_.store(false);
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }
    
    ~PeriodicMonitor() {
        stop();
    }
};

// Custom allocator for nanoflann that uses the memory tracker
template<typename T>
class TrackedAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    template<typename U>
    struct rebind {
        using other = TrackedAllocator<U>;
    };
    
    TrackedAllocator() = default;
    
    template<typename U>
    TrackedAllocator(const TrackedAllocator<U>&) noexcept {}
    
    pointer allocate(size_type n) {
        size_t bytes = n * sizeof(T);
        pointer ptr = static_cast<pointer>(std::malloc(bytes));
        
        if (ptr) {
            MemoryTracker::track_allocation(ptr, bytes);
        }
        
        return ptr;
    }
    
    void deallocate(pointer ptr, size_type n) noexcept {
        if (ptr) {
            MemoryTracker::track_deallocation(ptr);
            std::free(ptr);
        }
    }
    
    template<typename U, typename... Args>
    void construct(U* ptr, Args&&... args) {
        new(ptr) U(std::forward<Args>(args)...);
    }
    
    template<typename U>
    void destroy(U* ptr) {
        ptr->~U();
    }
};

template<typename T, typename U>
bool operator==(const TrackedAllocator<T>&, const TrackedAllocator<U>&) noexcept {
    return true;
}

template<typename T, typename U>
bool operator!=(const TrackedAllocator<T>&, const TrackedAllocator<U>&) noexcept {
    return false;
}

// Main memory monitor class with RAII
class MemoryMonitor {
private:
    PeriodicMonitor periodic_monitor_;
    bool was_monitoring_enabled_;
    
public:
    explicit MemoryMonitor(const MonitorConfig& config = MonitorConfig{}) {
        was_monitoring_enabled_ = MemoryTracker::is_monitoring_enabled();
        MemoryTracker::configure(config);
        MemoryTracker::enable_monitoring(true);
        
        if (config.enable_periodic_reports) {
            periodic_monitor_.start();
        }
    }
    
    ~MemoryMonitor() {
        periodic_monitor_.stop();
        MemoryTracker::enable_monitoring(was_monitoring_enabled_);
    }
    
    // Get current memory usage
    size_t get_current_usage() const {
        return MemoryTracker::get_stats().current_usage.load();
    }
    
    // Get peak memory usage
    size_t get_peak_usage() const {
        return MemoryTracker::get_stats().peak_usage.load();
    }
    
    // Check if threshold was exceeded
    bool threshold_exceeded() const {
        return MemoryTracker::get_stats().threshold_exceeded.load();
    }
    
    // Get full statistics
    MemoryStats get_stats() const {
        return MemoryTracker::get_stats();
    }
    
    // Reset statistics
    void reset() {
        MemoryTracker::reset_stats();
    }
    
    // Generate a memory report
    std::string generate_report() const {
        auto stats = get_stats();
        std::string report;
        
        report += "=== NanoFlann Memory Monitor Report ===\n";
        report += "Current Usage: " + std::to_string(stats.current_usage.load() / (1024.0 * 1024.0)) + " MB\n";
        report += "Peak Usage: " + std::to_string(stats.peak_usage.load() / (1024.0 * 1024.0)) + " MB\n";
        report += "Total Allocated: " + std::to_string(stats.total_allocations.load() / (1024.0 * 1024.0)) + " MB\n";
        report += "Allocation Count: " + std::to_string(stats.allocation_count.load()) + "\n";
        report += "Threshold Exceeded: " + std::string(stats.threshold_exceeded.load() ? "Yes" : "No") + "\n";
        report += "=======================================\n";
        
        return report;
    }
};

// Convenience macros for easy integration
#define NANOFLANN_MONITOR_START(threshold_mb) \
    do { \
        NanoFlannMemory::MonitorConfig config; \
        config.threshold_bytes = (threshold_mb) * 1024 * 1024; \
        static NanoFlannMemory::MemoryMonitor monitor(config); \
    } while(0)

#define NANOFLANN_MONITOR_REPORT() \
    do { \
        auto stats = NanoFlannMemory::MemoryTracker::get_stats(); \
        std::cerr << "[NANOFLANN] Current: " << (stats.current_usage.load() / (1024.0 * 1024.0)) << " MB, " \
                  << "Peak: " << (stats.peak_usage.load() / (1024.0 * 1024.0)) << " MB\n"; \
    } while(0)

#define NANOFLANN_MONITOR_RESET() \
    NanoFlannMemory::MemoryTracker::reset_stats()

} // namespace NanoFlannMemory

#endif // NANOFLANN_MEMORY_MONITOR_HPP
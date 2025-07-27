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
#include <sstream>
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
    
    // Get current statistics snapshot
    static void get_stats_snapshot(size_t& current, size_t& peak, size_t& total, size_t& count, bool& exceeded) {
        current = stats_.current_usage.load();
        peak = stats_.peak_usage.load();
        total = stats_.total_allocations.load();
        count = stats_.allocation_count.load();
        exceeded = stats_.threshold_exceeded.load();
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
                    size_t current, peak, total, count;
                    bool exceeded;
                    MemoryTracker::get_stats_snapshot(current, peak, total, count, exceeded);
                    
                    if (config.periodic_callback) {
                        config.periodic_callback(current, peak, count);
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

// Process memory monitoring utilities
class ProcessMemoryMonitor {
private:
    static size_t get_current_rss() {
        // Read from /proc/self/status on Linux
        std::ifstream status_file("/proc/self/status");
        std::string line;
        while (std::getline(status_file, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                std::istringstream iss(line);
                std::string name, value, unit;
                iss >> name >> value >> unit;
                
                size_t memory_kb = std::stoull(value);
                return memory_kb * 1024; // Convert KB to bytes
            }
        }
        return 0;
    }
    
public:
    static size_t get_memory_usage() {
        return get_current_rss();
    }
};

// Main memory monitor class with RAII
class MemoryMonitor {
private:
    PeriodicMonitor periodic_monitor_;
    bool was_monitoring_enabled_;
    size_t baseline_memory_;
    mutable size_t peak_memory_during_monitoring_;
    
public:
    explicit MemoryMonitor(const MonitorConfig& config = MonitorConfig{}) {
        was_monitoring_enabled_ = MemoryTracker::is_monitoring_enabled();
        MemoryTracker::configure(config);
        MemoryTracker::enable_monitoring(true);
        
        // Record baseline memory usage
        baseline_memory_ = ProcessMemoryMonitor::get_memory_usage();
        peak_memory_during_monitoring_ = baseline_memory_;
        
        if (config.enable_periodic_reports) {
            periodic_monitor_.start();
        }
    }
    
    ~MemoryMonitor() {
        periodic_monitor_.stop();
        MemoryTracker::enable_monitoring(was_monitoring_enabled_);
    }
    
    // Get current memory usage (process-based)
    size_t get_current_usage() const {
        size_t current = ProcessMemoryMonitor::get_memory_usage();
        return current > baseline_memory_ ? current - baseline_memory_ : 0;
    }
    
    // Get peak memory usage during monitoring
    size_t get_peak_usage() const {
        size_t current = ProcessMemoryMonitor::get_memory_usage();
        if (current > peak_memory_during_monitoring_) {
            peak_memory_during_monitoring_ = current;
        }
        return peak_memory_during_monitoring_ > baseline_memory_ ? 
               peak_memory_during_monitoring_ - baseline_memory_ : 0;
    }
    
    // Check if threshold was exceeded
    bool threshold_exceeded() const {
        return get_current_usage() > MemoryTracker::get_config().threshold_bytes;
    }
    
    // Get current process memory
    size_t get_total_process_memory() const {
        return ProcessMemoryMonitor::get_memory_usage();
    }
    
    // Reset statistics
    void reset() {
        baseline_memory_ = ProcessMemoryMonitor::get_memory_usage();
        peak_memory_during_monitoring_ = baseline_memory_;
        MemoryTracker::reset_stats();
    }
    
    // Generate a memory report
    std::string generate_report() const {
        size_t current_usage = get_current_usage();
        size_t peak_usage = get_peak_usage();
        size_t total_process = get_total_process_memory();
        
        std::string report;
        report += "=== NanoFlann Memory Monitor Report ===\n";
        report += "Memory Usage Since Monitor Start: " + std::to_string(current_usage / (1024.0 * 1024.0)) + " MB\n";
        report += "Peak Usage During Monitoring: " + std::to_string(peak_usage / (1024.0 * 1024.0)) + " MB\n";
        report += "Total Process Memory: " + std::to_string(total_process / (1024.0 * 1024.0)) + " MB\n";
        report += "Baseline Memory: " + std::to_string(baseline_memory_ / (1024.0 * 1024.0)) + " MB\n";
        report += "Threshold: " + std::to_string(MemoryTracker::get_config().threshold_bytes / (1024.0 * 1024.0)) + " MB\n";
        report += "Threshold Exceeded: " + std::string(threshold_exceeded() ? "Yes" : "No") + "\n";
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
        size_t current = NanoFlannMemory::ProcessMemoryMonitor::get_memory_usage(); \
        std::cerr << "[NANOFLANN] Process Memory: " << (current / (1024.0 * 1024.0)) << " MB\n"; \
    } while(0)

#define NANOFLANN_MONITOR_RESET() \
    NanoFlannMemory::MemoryTracker::reset_stats()

} // namespace NanoFlannMemory

#endif // NANOFLANN_MEMORY_MONITOR_HPP
#ifndef NANOFLANN_MEMORY_MONITOR_HPP
#define NANOFLANN_MEMORY_MONITOR_HPP

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>

#ifdef __linux__
#include <fstream>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#endif

namespace NanoflannMonitor {

// Forward declarations
class MemoryMonitor;
class ScopedMemoryMonitor;

// Configuration structure
struct MonitorConfig {
    size_t threshold_mb = 100;          // Memory threshold in MB
    size_t check_interval_ms = 100;     // How often to check memory (milliseconds)
    bool monitor_rss = true;            // Monitor RSS (Resident Set Size)
    bool monitor_vss = false;           // Monitor VSS (Virtual Set Size)
    bool print_to_stderr = true;        // Print to stderr
    std::function<void(const std::string&)> custom_logger = nullptr;  // Custom logging function
};

// Memory statistics
struct MemoryStats {
    size_t rss_bytes = 0;       // Resident Set Size in bytes
    size_t vss_bytes = 0;       // Virtual Set Size in bytes
    size_t peak_rss_bytes = 0;  // Peak RSS during monitoring
    std::chrono::steady_clock::time_point timestamp;
};

// Platform-specific memory reading functions
namespace PlatformMemory {
    
#ifdef __linux__
    inline MemoryStats get_current_memory() {
        MemoryStats stats;
        stats.timestamp = std::chrono::steady_clock::now();
        
        std::ifstream status("/proc/self/status");
        std::string line;
        
        while (std::getline(status, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                std::istringstream iss(line.substr(6));
                size_t rss_kb;
                iss >> rss_kb;
                stats.rss_bytes = rss_kb * 1024;
            } else if (line.substr(0, 6) == "VmSize") {
                std::istringstream iss(line.substr(7));
                size_t vss_kb;
                iss >> vss_kb;
                stats.vss_bytes = vss_kb * 1024;
            }
        }
        
        return stats;
    }
    
#elif defined(_WIN32)
    inline MemoryStats get_current_memory() {
        MemoryStats stats;
        stats.timestamp = std::chrono::steady_clock::now();
        
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), 
                                (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            stats.rss_bytes = pmc.WorkingSetSize;
            stats.vss_bytes = pmc.PrivateUsage;
        }
        
        return stats;
    }
    
#elif defined(__APPLE__)
    inline MemoryStats get_current_memory() {
        MemoryStats stats;
        stats.timestamp = std::chrono::steady_clock::now();
        
        struct task_basic_info info;
        mach_msg_type_number_t info_count = TASK_BASIC_INFO_COUNT;
        
        if (task_info(mach_task_self(), TASK_BASIC_INFO, 
                     (task_info_t)&info, &info_count) == KERN_SUCCESS) {
            stats.rss_bytes = info.resident_size;
            stats.vss_bytes = info.virtual_size;
        }
        
        return stats;
    }
    
#else
    inline MemoryStats get_current_memory() {
        return MemoryStats{};  // Unsupported platform
    }
#endif

} // namespace PlatformMemory

// Main memory monitor class
class MemoryMonitor {
private:
    MonitorConfig config_;
    std::atomic<bool> monitoring_{false};
    std::atomic<bool> should_stop_{false};
    std::unique_ptr<std::thread> monitor_thread_;
    MemoryStats baseline_stats_;
    MemoryStats peak_stats_;
    std::atomic<size_t> threshold_exceeded_count_{0};
    
    void log_message(const std::string& message) {
        if (config_.custom_logger) {
            config_.custom_logger(message);
        } else if (config_.print_to_stderr) {
            std::cerr << message << std::endl;
        }
    }
    
    void monitor_loop() {
        while (!should_stop_.load()) {
            auto current = PlatformMemory::get_current_memory();
            
            // Update peak RSS
            if (current.rss_bytes > peak_stats_.rss_bytes) {
                peak_stats_ = current;
            }
            
            // Check thresholds
            size_t threshold_bytes = config_.threshold_mb * 1024 * 1024;
            
            if (config_.monitor_rss && current.rss_bytes > threshold_bytes) {
                threshold_exceeded_count_++;
                
                std::ostringstream oss;
                oss << "[NANOFLANN MONITOR] Memory threshold exceeded!\n"
                    << "  Current RSS: " << std::fixed << std::setprecision(2) 
                    << (current.rss_bytes / (1024.0 * 1024.0)) << " MB\n"
                    << "  Threshold: " << config_.threshold_mb << " MB\n"
                    << "  Exceeded by: " << std::fixed << std::setprecision(2)
                    << ((current.rss_bytes - threshold_bytes) / (1024.0 * 1024.0)) << " MB\n"
                    << "  Times exceeded: " << threshold_exceeded_count_.load();
                
                if (config_.monitor_vss) {
                    oss << "\n  Current VSS: " << std::fixed << std::setprecision(2)
                        << (current.vss_bytes / (1024.0 * 1024.0)) << " MB";
                }
                
                log_message(oss.str());
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.check_interval_ms));
        }
    }
    
public:
    explicit MemoryMonitor(const MonitorConfig& config = MonitorConfig{})
        : config_(config) {}
    
    ~MemoryMonitor() {
        stop();
    }
    
    // Start monitoring
    void start() {
        if (monitoring_.load()) return;
        
        baseline_stats_ = PlatformMemory::get_current_memory();
        peak_stats_ = baseline_stats_;
        threshold_exceeded_count_ = 0;
        should_stop_ = false;
        monitoring_ = true;
        
        monitor_thread_ = std::make_unique<std::thread>(&MemoryMonitor::monitor_loop, this);
        
        std::ostringstream oss;
        oss << "[NANOFLANN MONITOR] Started monitoring\n"
            << "  Baseline RSS: " << std::fixed << std::setprecision(2)
            << (baseline_stats_.rss_bytes / (1024.0 * 1024.0)) << " MB\n"
            << "  Threshold: " << config_.threshold_mb << " MB\n"
            << "  Check interval: " << config_.check_interval_ms << " ms";
        
        log_message(oss.str());
    }
    
    // Stop monitoring
    void stop() {
        if (!monitoring_.load()) return;
        
        should_stop_ = true;
        if (monitor_thread_ && monitor_thread_->joinable()) {
            monitor_thread_->join();
        }
        monitoring_ = false;
        
        auto final_stats = PlatformMemory::get_current_memory();
        
        std::ostringstream oss;
        oss << "[NANOFLANN MONITOR] Stopped monitoring\n"
            << "  Final RSS: " << std::fixed << std::setprecision(2)
            << (final_stats.rss_bytes / (1024.0 * 1024.0)) << " MB\n"
            << "  Peak RSS: " << std::fixed << std::setprecision(2)
            << (peak_stats_.rss_bytes / (1024.0 * 1024.0)) << " MB\n"
            << "  Memory growth: " << std::fixed << std::setprecision(2)
            << ((final_stats.rss_bytes - baseline_stats_.rss_bytes) / (1024.0 * 1024.0)) << " MB\n"
            << "  Threshold exceeded: " << threshold_exceeded_count_.load() << " times";
        
        log_message(oss.str());
    }
    
    // Get current statistics
    MemoryStats get_current_stats() const {
        return PlatformMemory::get_current_memory();
    }
    
    MemoryStats get_peak_stats() const {
        return peak_stats_;
    }
    
    size_t get_threshold_exceeded_count() const {
        return threshold_exceeded_count_.load();
    }
    
    bool is_monitoring() const {
        return monitoring_.load();
    }
};

// RAII wrapper for scoped monitoring
class ScopedMemoryMonitor {
private:
    MemoryMonitor monitor_;
    std::string scope_name_;
    MonitorConfig config_;
    
public:
    ScopedMemoryMonitor(const std::string& scope_name, 
                       const MonitorConfig& config = MonitorConfig{})
        : monitor_(config), scope_name_(scope_name), config_(config) {
        
        if (config_.custom_logger) {
            config_.custom_logger("[NANOFLANN MONITOR] Entering scope: " + scope_name);
        } else if (config_.print_to_stderr) {
            std::cerr << "[NANOFLANN MONITOR] Entering scope: " << scope_name << std::endl;
        }
        
        monitor_.start();
    }
    
    ~ScopedMemoryMonitor() {
        monitor_.stop();
        
        if (config_.custom_logger) {
            config_.custom_logger("[NANOFLANN MONITOR] Exiting scope: " + scope_name_);
        } else if (config_.print_to_stderr) {
            std::cerr << "[NANOFLANN MONITOR] Exiting scope: " << scope_name_ << std::endl;
        }
    }
    
    // Delete copy/move operations
    ScopedMemoryMonitor(const ScopedMemoryMonitor&) = delete;
    ScopedMemoryMonitor& operator=(const ScopedMemoryMonitor&) = delete;
    ScopedMemoryMonitor(ScopedMemoryMonitor&&) = delete;
    ScopedMemoryMonitor& operator=(ScopedMemoryMonitor&&) = delete;
};

// Convenience function for one-shot memory measurement
inline MemoryStats measure_memory_usage(std::function<void()> operation,
                                       const std::string& operation_name = "operation") {
    auto before = PlatformMemory::get_current_memory();
    operation();
    auto after = PlatformMemory::get_current_memory();
    
    std::ostringstream oss;
    oss << "[NANOFLANN MONITOR] Memory usage for " << operation_name << ":\n"
        << "  RSS growth: " << std::fixed << std::setprecision(2)
        << ((after.rss_bytes - before.rss_bytes) / (1024.0 * 1024.0)) << " MB\n"
        << "  Final RSS: " << std::fixed << std::setprecision(2)
        << (after.rss_bytes / (1024.0 * 1024.0)) << " MB";
    
    std::cerr << oss.str() << std::endl;
    
    return after;
}

} // namespace NanoflannMonitor

#endif // NANOFLANN_MEMORY_MONITOR_HPP
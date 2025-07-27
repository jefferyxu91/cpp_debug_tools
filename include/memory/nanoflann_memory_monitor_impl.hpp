#ifndef NANOFLANN_MEMORY_MONITOR_IMPL_HPP
#define NANOFLANN_MEMORY_MONITOR_IMPL_HPP

#include "nanoflann_memory_monitor.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>

// Platform-specific includes for memory usage detection
#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <sys/resource.h>
    #include <fstream>
#elif defined(__APPLE__)
    #include <mach/mach.h>
    #include <sys/sysctl.h>
#endif

namespace nanoflann {

// MemoryMonitor implementation
MemoryMonitor::MemoryMonitor(const Config& config, MemoryReporter reporter)
    : config_(config), memory_reporter_(reporter ? reporter : get_process_memory_usage) {
    stats_.last_check = std::chrono::steady_clock::now();
    stats_.peak_time = stats_.last_check;
}

MemoryMonitor::~MemoryMonitor() {
    stop();
}

void MemoryMonitor::start() {
    if (monitoring_active_.load()) {
        return; // Already active
    }
    
    monitoring_active_.store(true);
    
    if (config_.enable_background_monitoring) {
        monitor_thread_ = std::make_unique<std::thread>(&MemoryMonitor::monitoring_loop, this);
    }
    
    // Initial memory check
    check_memory("Monitor started");
}

void MemoryMonitor::stop() {
    if (!monitoring_active_.load()) {
        return; // Not active
    }
    
    monitoring_active_.store(false);
    monitor_cv_.notify_all();
    
    if (monitor_thread_ && monitor_thread_->joinable()) {
        monitor_thread_->join();
    }
    
    // Final memory check
    check_memory("Monitor stopped");
}

void MemoryMonitor::set_threshold(size_t threshold_mb) {
    config_.memory_threshold_mb = threshold_mb;
}

MemoryMonitor::MemoryStats MemoryMonitor::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

std::vector<MemoryMonitor::MemoryEvent> MemoryMonitor::get_event_history() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return event_history_;
}

void MemoryMonitor::add_callback(MemoryCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    callbacks_.push_back(std::move(callback));
}

void MemoryMonitor::clear_callbacks() {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    callbacks_.clear();
}

void MemoryMonitor::check_memory(const std::string& context) {
    if (!monitoring_active_.load()) {
        return;
    }
    
    process_memory_check(context);
}

void MemoryMonitor::mark_tree_build_start(const std::string& context) {
    MemoryEvent event{
        EventType::TREE_BUILD_START,
        stats_.current_memory_mb,
        std::chrono::steady_clock::now(),
        context,
        get_stack_trace()
    };
    
    trigger_event(event);
    check_memory("Tree build start: " + context);
}

void MemoryMonitor::mark_tree_build_end(const std::string& context) {
    MemoryEvent event{
        EventType::TREE_BUILD_END,
        stats_.current_memory_mb,
        std::chrono::steady_clock::now(),
        context,
        get_stack_trace()
    };
    
    trigger_event(event);
    check_memory("Tree build end: " + context);
}

std::unordered_map<std::string, size_t> MemoryMonitor::get_context_memory_usage() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return context_memory_usage_;
}

void MemoryMonitor::reset() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = MemoryStats{};
    event_history_.clear();
    context_memory_usage_.clear();
    stats_.last_check = std::chrono::steady_clock::now();
    stats_.peak_time = stats_.last_check;
}

void MemoryMonitor::monitoring_loop() {
    while (monitoring_active_.load()) {
        check_memory("Background check");
        
        // Wait for next check interval or stop signal
        std::unique_lock<std::mutex> lock(stats_mutex_);
        if (monitor_cv_.wait_for(lock, 
                                std::chrono::milliseconds(config_.check_interval_ms),
                                [this] { return !monitoring_active_.load(); })) {
            break; // Stop signal received
        }
    }
}

void MemoryMonitor::process_memory_check(const std::string& context) {
    size_t current_memory_bytes = memory_reporter_();
    size_t current_memory_mb = current_memory_bytes / (1024 * 1024);
    
    update_stats(current_memory_mb);
    
    // Check for threshold exceeded
    if (current_memory_mb > config_.memory_threshold_mb) {
        MemoryEvent event{
            EventType::THRESHOLD_EXCEEDED,
            current_memory_mb,
            std::chrono::steady_clock::now(),
            context,
            get_stack_trace()
        };
        
        trigger_event(event);
    }
    
    // Check for memory spike (sudden increase)
    static size_t last_memory_mb = 0;
    static std::chrono::steady_clock::time_point last_spike_check = std::chrono::steady_clock::now();
    
    auto now = std::chrono::steady_clock::now();
    auto time_since_last_check = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_spike_check);
    
    if (time_since_last_check.count() > 1000) { // Check every second
        size_t memory_increase = (current_memory_mb > last_memory_mb) ? 
                                (current_memory_mb - last_memory_mb) : 0;
        
        if (memory_increase > 50) { // 50MB spike threshold
            MemoryEvent event{
                EventType::MEMORY_SPIKE_DETECTED,
                current_memory_mb,
                now,
                context + " (spike: +" + std::to_string(memory_increase) + "MB)",
                get_stack_trace()
            };
            
            trigger_event(event);
        }
        
        last_memory_mb = current_memory_mb;
        last_spike_check = now;
    }
}

void MemoryMonitor::trigger_event(const MemoryEvent& event) {
    // Store event in history
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        event_history_.push_back(event);
        
        // Keep only last 1000 events to prevent memory bloat
        if (event_history_.size() > 1000) {
            event_history_.erase(event_history_.begin(), 
                               event_history_.begin() + (event_history_.size() - 1000));
        }
    }
    
    // Trigger callbacks
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    for (const auto& callback : callbacks_) {
        try {
            callback(event);
        } catch (...) {
            // Ignore callback errors to prevent monitoring from breaking
        }
    }
}

std::string MemoryMonitor::get_stack_trace() const {
    // Simplified stack trace - in a real implementation, you might want to use
    // libraries like libbacktrace or platform-specific APIs
    return "Stack trace not available in this implementation";
}

void MemoryMonitor::update_stats(size_t current_memory_mb) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.current_memory_mb = current_memory_mb;
    stats_.last_check = std::chrono::steady_clock::now();
    
    if (current_memory_mb > stats_.peak_memory_mb) {
        stats_.peak_memory_mb = current_memory_mb;
        stats_.peak_time = stats_.last_check;
    }
}

// Platform-specific memory usage detection
size_t MemoryMonitor::get_process_memory_usage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
    
#elif defined(__linux__)
    std::ifstream status_file("/proc/self/status");
    std::string line;
    while (std::getline(status_file, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            size_t pos = line.find_first_not_of(" \t", 6);
            if (pos != std::string::npos) {
                size_t kb = std::stoul(line.substr(pos));
                return kb * 1024; // Convert KB to bytes
            }
        }
    }
    return 0;
    
#elif defined(__APPLE__)
    struct task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;
    
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count) == KERN_SUCCESS) {
        return t_info.resident_size;
    }
    return 0;
    
#else
    // Fallback for other platforms
    return 0;
#endif
}

// TreeBuildScope implementation
TreeBuildScope::TreeBuildScope(MemoryMonitor& monitor, const std::string& context)
    : monitor_(monitor), context_(context) {
    monitor_.mark_tree_build_start(context_);
}

TreeBuildScope::~TreeBuildScope() {
    if (!ended_) {
        end();
    }
}

void TreeBuildScope::end(const std::string& context) {
    if (!ended_) {
        monitor_.mark_tree_build_end(context.empty() ? context_ : context);
        ended_ = true;
    }
}

// memory_utils implementation
namespace memory_utils {

MemoryMonitor create_default_monitor() {
    MemoryMonitor::Config config;
    config.memory_threshold_mb = 100;  // 100MB default threshold
    config.check_interval_ms = 100;    // Check every 100ms
    config.enable_background_monitoring = true;
    config.enable_detailed_logging = false;
    config.log_prefix = "[NANOFLANN_MEMORY]";
    
    return MemoryMonitor(config);
}

MemoryMonitor create_large_scale_monitor(size_t expected_data_size_mb) {
    MemoryMonitor::Config config;
    config.memory_threshold_mb = expected_data_size_mb * 3; // 3x expected size as threshold
    config.check_interval_ms = 50;     // More frequent checks for large operations
    config.enable_background_monitoring = true;
    config.enable_detailed_logging = true;
    config.log_prefix = "[NANOFLANN_LARGE_SCALE]";
    
    return MemoryMonitor(config);
}

void add_standard_logging(MemoryMonitor& monitor, 
                         std::function<void(const std::string&)> log_function) {
    if (!log_function) {
        log_function = [](const std::string& message) {
            std::cerr << message << std::endl;
        };
    }
    
    monitor.add_callback([log_function](const MemoryMonitor::MemoryEvent& event) {
        std::ostringstream oss;
        
        auto time_t = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        auto tm = *std::localtime(&time_t);
        
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " ";
        
        switch (event.type) {
            case MemoryMonitor::EventType::THRESHOLD_EXCEEDED:
                oss << "[WARNING] Memory threshold exceeded: " << event.memory_mb << "MB";
                break;
            case MemoryMonitor::EventType::PEAK_MEMORY_REACHED:
                oss << "[INFO] Peak memory reached: " << event.memory_mb << "MB";
                break;
            case MemoryMonitor::EventType::TREE_BUILD_START:
                oss << "[INFO] Tree build started: " << event.context;
                break;
            case MemoryMonitor::EventType::TREE_BUILD_END:
                oss << "[INFO] Tree build ended: " << event.context;
                break;
            case MemoryMonitor::EventType::MEMORY_SPIKE_DETECTED:
                oss << "[WARNING] Memory spike detected: " << event.context;
                break;
        }
        
        if (!event.context.empty()) {
            oss << " (Context: " << event.context << ")";
        }
        
        log_function(oss.str());
    });
}

size_t estimate_tree_memory_usage(size_t num_points, 
                                 size_t dimension, 
                                 size_t point_type_size) {
    // Rough estimation for nanoflann tree memory usage
    // This includes:
    // - Point data storage
    // - Tree node structures
    // - Index arrays
    // - Temporary buffers during construction
    
    size_t point_data_size = num_points * dimension * point_type_size;
    size_t tree_nodes_size = num_points * sizeof(void*) * 2; // Rough estimate for tree nodes
    size_t index_arrays_size = num_points * sizeof(size_t) * 2; // Index arrays
    size_t construction_buffer_size = num_points * sizeof(size_t); // Temporary buffers
    
    size_t total_bytes = point_data_size + tree_nodes_size + 
                        index_arrays_size + construction_buffer_size;
    
    // Add 20% overhead for safety
    total_bytes = static_cast<size_t>(total_bytes * 1.2);
    
    return total_bytes / (1024 * 1024); // Convert to MB
}

} // namespace memory_utils

} // namespace nanoflann

#endif // NANOFLANN_MEMORY_MONITOR_IMPL_HPP
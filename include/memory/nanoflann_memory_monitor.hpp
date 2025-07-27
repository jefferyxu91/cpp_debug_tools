#ifndef NANOFLANN_MEMORY_MONITOR_HPP
#define NANOFLANN_MEMORY_MONITOR_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <chrono>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <unordered_map>

namespace nanoflann {

/**
 * @brief Memory monitoring tool specifically designed for nanoflann tree building operations
 * 
 * This tool provides non-intrusive memory monitoring with minimal overhead,
 * focusing on detecting memory spikes during tree construction and indexing.
 */
class MemoryMonitor {
public:
    // Configuration
    struct Config {
        size_t memory_threshold_mb = 100;           // Memory threshold in MB
        size_t check_interval_ms = 100;             // Memory check interval in milliseconds
        bool enable_background_monitoring = true;   // Enable background thread monitoring
        bool enable_detailed_logging = false;       // Enable detailed memory logging
        std::string log_prefix = "[NANOFLANN_MEMORY]"; // Log message prefix
    };

    // Memory usage statistics
    struct MemoryStats {
        size_t peak_memory_mb = 0;
        size_t current_memory_mb = 0;
        size_t allocation_count = 0;
        size_t deallocation_count = 0;
        std::chrono::steady_clock::time_point last_check;
        std::chrono::steady_clock::time_point peak_time;
    };

    // Memory event types
    enum class EventType {
        THRESHOLD_EXCEEDED,
        PEAK_MEMORY_REACHED,
        TREE_BUILD_START,
        TREE_BUILD_END,
        MEMORY_SPIKE_DETECTED
    };

    // Memory event structure
    struct MemoryEvent {
        EventType type;
        size_t memory_mb;
        std::chrono::steady_clock::time_point timestamp;
        std::string context;
        std::string stack_trace;
    };

    using MemoryCallback = std::function<void(const MemoryEvent&)>;
    using MemoryReporter = std::function<size_t()>; // Returns current memory usage in bytes

private:
    Config config_;
    MemoryStats stats_;
    std::atomic<bool> monitoring_active_{false};
    std::unique_ptr<std::thread> monitor_thread_;
    std::mutex stats_mutex_;
    std::mutex callbacks_mutex_;
    std::condition_variable monitor_cv_;
    
    std::vector<MemoryCallback> callbacks_;
    std::vector<MemoryEvent> event_history_;
    std::unordered_map<std::string, size_t> context_memory_usage_;
    
    MemoryReporter memory_reporter_;
    
    // Default memory reporter (platform-specific)
    static size_t get_process_memory_usage();

public:
    /**
     * @brief Constructor with optional configuration
     * @param config Memory monitoring configuration
     * @param reporter Optional custom memory reporter function
     */
    explicit MemoryMonitor(const Config& config = Config{}, 
                          MemoryReporter reporter = nullptr);
    
    /**
     * @brief Destructor - stops monitoring if active
     */
    ~MemoryMonitor();
    
    /**
     * @brief Start memory monitoring
     */
    void start();
    
    /**
     * @brief Stop memory monitoring
     */
    void stop();
    
    /**
     * @brief Check if monitoring is active
     * @return true if monitoring is active
     */
    bool is_active() const { return monitoring_active_.load(); }
    
    /**
     * @brief Set memory threshold
     * @param threshold_mb Memory threshold in MB
     */
    void set_threshold(size_t threshold_mb);
    
    /**
     * @brief Get current memory threshold
     * @return Memory threshold in MB
     */
    size_t get_threshold() const { return config_.memory_threshold_mb; }
    
    /**
     * @brief Get current memory statistics
     * @return Current memory statistics
     */
    MemoryStats get_stats() const;
    
    /**
     * @brief Get memory event history
     * @return Vector of memory events
     */
    std::vector<MemoryEvent> get_event_history() const;
    
    /**
     * @brief Add memory event callback
     * @param callback Function to call when memory events occur
     */
    void add_callback(MemoryCallback callback);
    
    /**
     * @brief Clear all callbacks
     */
    void clear_callbacks();
    
    /**
     * @brief Manually trigger a memory check
     * @param context Optional context string for the check
     */
    void check_memory(const std::string& context = "");
    
    /**
     * @brief Mark the start of a tree building operation
     * @param context Context information about the tree building
     */
    void mark_tree_build_start(const std::string& context = "");
    
    /**
     * @brief Mark the end of a tree building operation
     * @param context Context information about the tree building
     */
    void mark_tree_build_end(const std::string& context = "");
    
    /**
     * @brief Get memory usage by context
     * @return Map of context to memory usage
     */
    std::unordered_map<std::string, size_t> get_context_memory_usage() const;
    
    /**
     * @brief Reset all statistics and event history
     */
    void reset();

private:
    /**
     * @brief Background monitoring thread function
     */
    void monitoring_loop();
    
    /**
     * @brief Process memory check and trigger events if needed
     * @param context Context for the memory check
     */
    void process_memory_check(const std::string& context = "");
    
    /**
     * @brief Trigger memory event callbacks
     * @param event Memory event to trigger
     */
    void trigger_event(const MemoryEvent& event);
    
    /**
     * @brief Get stack trace (platform-specific implementation)
     * @return Stack trace string
     */
    std::string get_stack_trace() const;
    
    /**
     * @brief Update memory statistics
     * @param current_memory_mb Current memory usage in MB
     */
    void update_stats(size_t current_memory_mb);
};

/**
 * @brief RAII wrapper for tree building operations
 * 
 * Automatically marks the start and end of tree building operations
 * when used with scope-based lifetime management.
 */
class TreeBuildScope {
private:
    MemoryMonitor& monitor_;
    std::string context_;
    bool ended_ = false;

public:
    /**
     * @brief Constructor - marks tree build start
     * @param monitor Memory monitor instance
     * @param context Context information
     */
    TreeBuildScope(MemoryMonitor& monitor, const std::string& context = "");
    
    /**
     * @brief Destructor - marks tree build end
     */
    ~TreeBuildScope();
    
    /**
     * @brief Manually end the tree build scope
     * @param context Optional context for the end event
     */
    void end(const std::string& context = "");
    
    // Disable copying
    TreeBuildScope(const TreeBuildScope&) = delete;
    TreeBuildScope& operator=(const TreeBuildScope&) = delete;
};

/**
 * @brief Utility functions for nanoflann memory monitoring
 */
namespace memory_utils {
    
    /**
     * @brief Create a default memory monitor with nanoflann-optimized settings
     * @return Configured memory monitor instance
     */
    MemoryMonitor create_default_monitor();
    
    /**
     * @brief Create a memory monitor for large-scale tree building
     * @param expected_data_size Expected size of data in MB
     * @return Configured memory monitor instance
     */
    MemoryMonitor create_large_scale_monitor(size_t expected_data_size_mb);
    
    /**
     * @brief Add standard logging callback to memory monitor
     * @param monitor Memory monitor instance
     * @param log_function Custom log function (defaults to std::cerr)
     */
    void add_standard_logging(MemoryMonitor& monitor, 
                             std::function<void(const std::string&)> log_function = nullptr);
    
    /**
     * @brief Estimate memory usage for nanoflann tree building
     * @param num_points Number of points
     * @param dimension Dimension of points
     * @param point_type_size Size of point type in bytes
     * @return Estimated memory usage in MB
     */
    size_t estimate_tree_memory_usage(size_t num_points, 
                                     size_t dimension, 
                                     size_t point_type_size = sizeof(double));
}

} // namespace nanoflann

// Implementation
#include "nanoflann_memory_monitor_impl.hpp"

#endif // NANOFLANN_MEMORY_MONITOR_HPP
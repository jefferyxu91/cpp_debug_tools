#ifndef NANOFLANN_MONITORED_INDEX_HPP
#define NANOFLANN_MONITORED_INDEX_HPP

#include <nanoflann.hpp>
#include <memory/nanoflann_memory_monitor.hpp>
#include <iostream>
#include <functional>

namespace nanoflann {

/**
 * @brief Monitored KD-tree index that automatically tracks memory usage during tree building
 * 
 * This class inherits from nanoflann's KDTreeSingleIndexAdaptor and automatically
 * monitors memory usage during tree construction. It will automatically print
 * warnings when memory usage exceeds configured thresholds.
 * 
 * @tparam Distance Distance metric adaptor
 * @tparam DatasetAdaptor Dataset adaptor
 * @tparam DIM Dimension (use -1 for dynamic)
 * @tparam index_t Index type
 */
template <typename Distance, typename DatasetAdaptor, int DIM = -1, typename index_t = uint32_t>
class MonitoredKDTreeIndex : public KDTreeSingleIndexAdaptor<Distance, DatasetAdaptor, DIM, index_t> {
public:
    using Base = KDTreeSingleIndexAdaptor<Distance, DatasetAdaptor, DIM, index_t>;
    using ElementType = typename Base::ElementType;
    using DistanceType = typename Base::DistanceType;
    using IndexType = typename Base::IndexType;
    using Size = typename Base::Size;
    using Dimension = typename Base::Dimension;

    // Configuration for memory monitoring
    struct MonitorConfig {
        size_t memory_threshold_mb = 100;           // Memory threshold in MB
        bool enable_auto_monitoring = true;         // Enable automatic monitoring
        bool print_warnings = true;                 // Print warnings to std::cerr
        std::function<void(const std::string&)> custom_logger = nullptr; // Custom logger function
        std::string context_prefix = "KDTree";      // Context prefix for logging
        
        MonitorConfig() = default;
        
        MonitorConfig(size_t threshold_mb, bool auto_monitor = true, bool print_warn = true)
            : memory_threshold_mb(threshold_mb), enable_auto_monitoring(auto_monitor), print_warnings(print_warn) {}
    };

private:
    MonitorConfig monitor_config_;
    std::unique_ptr<MemoryMonitor> memory_monitor_;
    bool monitoring_active_ = false;

    // Default logger function
    void default_logger(const std::string& message) {
        if (monitor_config_.print_warnings) {
            std::cerr << "[NANOFLANN_MONITORED] " << message << std::endl;
        }
    }

    // Setup memory monitoring
    void setup_monitoring() {
        if (!monitor_config_.enable_auto_monitoring) {
            return;
        }

        MemoryMonitor::Config config;
        config.memory_threshold_mb = monitor_config_.memory_threshold_mb;
        config.check_interval_ms = 50;  // Check every 50ms during tree building
        config.enable_background_monitoring = false; // No background monitoring for this use case
        config.enable_detailed_logging = false;
        config.log_prefix = "[NANOFLANN_MONITORED]";

        memory_monitor_ = std::make_unique<MemoryMonitor>(config);
        
        // Add callback for threshold exceeded events
        memory_monitor_->add_callback([this](const MemoryMonitor::MemoryEvent& event) {
            if (event.type == MemoryMonitor::EventType::THRESHOLD_EXCEEDED) {
                std::string message = "Memory threshold exceeded during tree building: " + 
                                    std::to_string(event.memory_mb) + "MB (threshold: " + 
                                    std::to_string(monitor_config_.memory_threshold_mb) + "MB)";
                
                if (monitor_config_.custom_logger) {
                    monitor_config_.custom_logger(message);
                } else {
                    default_logger(message);
                }
            }
        });

        monitoring_active_ = true;
    }

public:
    /**
     * @brief Constructor with memory monitoring configuration
     * 
     * @param dimensionality Tree dimensionality
     * @param inputData Dataset adaptor
     * @param params Tree construction parameters
     * @param monitor_config Memory monitoring configuration
     * @param args Additional arguments for distance metric
     */
    template <class... Args>
    explicit MonitoredKDTreeIndex(
        const Dimension dimensionality,
        const DatasetAdaptor& inputData,
        const KDTreeSingleIndexAdaptorParams& params,
        const MonitorConfig& monitor_config = MonitorConfig{},
        Args&&... args)
        : Base(dimensionality, inputData, params, std::forward<Args>(args)...),
          monitor_config_(monitor_config) {
        setup_monitoring();
    }

    /**
     * @brief Constructor with default parameters
     * 
     * @param dimensionality Tree dimensionality
     * @param inputData Dataset adaptor
     * @param monitor_config Memory monitoring configuration
     */
    explicit MonitoredKDTreeIndex(
        const Dimension dimensionality,
        const DatasetAdaptor& inputData,
        const MonitorConfig& monitor_config = MonitorConfig{})
        : Base(dimensionality, inputData),
          monitor_config_(monitor_config) {
        setup_monitoring();
    }

    /**
     * @brief Constructor with custom memory threshold
     * 
     * @param dimensionality Tree dimensionality
     * @param inputData Dataset adaptor
     * @param memory_threshold_mb Memory threshold in MB
     * @param params Tree construction parameters
     */
    explicit MonitoredKDTreeIndex(
        const Dimension dimensionality,
        const DatasetAdaptor& inputData,
        size_t memory_threshold_mb,
        const KDTreeSingleIndexAdaptorParams& params = {})
        : Base(dimensionality, inputData, params),
          monitor_config_(memory_threshold_mb) {
        setup_monitoring();
    }

    /**
     * @brief Destructor
     */
    ~MonitoredKDTreeIndex() {
        if (memory_monitor_) {
            memory_monitor_->stop();
        }
    }

    /**
     * @brief Override buildIndex to add memory monitoring
     */
    void buildIndex() {
        if (!monitoring_active_ || !memory_monitor_) {
            Base::buildIndex();
            return;
        }

        // Start monitoring
        memory_monitor_->start();
        
        // Mark tree build start
        memory_monitor_->mark_tree_build_start(monitor_config_.context_prefix + " buildIndex");
        
        try {
            // Call the base class buildIndex method
            Base::buildIndex();
            
            // Mark tree build end
            memory_monitor_->mark_tree_build_end(monitor_config_.context_prefix + " buildIndex completed");
            
            // Print final statistics if enabled
            if (monitor_config_.print_warnings || monitor_config_.custom_logger) {
                auto stats = memory_monitor_->get_stats();
                std::string message = "Tree building completed. Peak memory: " + 
                                    std::to_string(stats.peak_memory_mb) + "MB";
                
                if (monitor_config_.custom_logger) {
                    monitor_config_.custom_logger(message);
                } else {
                    default_logger(message);
                }
            }
        } catch (...) {
            // Mark tree build end with error
            memory_monitor_->mark_tree_build_end(monitor_config_.context_prefix + " buildIndex failed");
            memory_monitor_->stop();
            throw; // Re-throw the exception
        }
        
        // Stop monitoring
        memory_monitor_->stop();
    }

    /**
     * @brief Get memory monitoring statistics
     * @return Memory statistics (only valid after buildIndex)
     */
    MemoryMonitor::MemoryStats get_memory_stats() const {
        if (memory_monitor_) {
            return memory_monitor_->get_stats();
        }
        return MemoryMonitor::MemoryStats{};
    }

    /**
     * @brief Get memory event history
     * @return Vector of memory events
     */
    std::vector<MemoryMonitor::MemoryEvent> get_memory_events() const {
        if (memory_monitor_) {
            return memory_monitor_->get_event_history();
        }
        return std::vector<MemoryMonitor::MemoryEvent>{};
    }

    /**
     * @brief Set memory threshold
     * @param threshold_mb New memory threshold in MB
     */
    void set_memory_threshold(size_t threshold_mb) {
        monitor_config_.memory_threshold_mb = threshold_mb;
        if (memory_monitor_) {
            memory_monitor_->set_threshold(threshold_mb);
        }
    }

    /**
     * @brief Get current memory threshold
     * @return Current memory threshold in MB
     */
    size_t get_memory_threshold() const {
        return monitor_config_.memory_threshold_mb;
    }

    /**
     * @brief Enable or disable memory monitoring
     * @param enable Whether to enable monitoring
     */
    void set_monitoring_enabled(bool enable) {
        monitor_config_.enable_auto_monitoring = enable;
    }

    /**
     * @brief Check if monitoring is enabled
     * @return true if monitoring is enabled
     */
    bool is_monitoring_enabled() const {
        return monitor_config_.enable_auto_monitoring;
    }

    /**
     * @brief Set custom logger function
     * @param logger Custom logger function
     */
    void set_custom_logger(std::function<void(const std::string&)> logger) {
        monitor_config_.custom_logger = logger;
    }

    /**
     * @brief Set context prefix for logging
     * @param prefix Context prefix
     */
    void set_context_prefix(const std::string& prefix) {
        monitor_config_.context_prefix = prefix;
    }

    // Disable copy constructor and assignment
    MonitoredKDTreeIndex(const MonitoredKDTreeIndex&) = delete;
    MonitoredKDTreeIndex& operator=(const MonitoredKDTreeIndex&) = delete;

    // Allow move constructor and assignment
    MonitoredKDTreeIndex(MonitoredKDTreeIndex&&) = default;
    MonitoredKDTreeIndex& operator=(MonitoredKDTreeIndex&&) = default;
};

/**
 * @brief Utility functions for creating monitored KD-tree indices
 */
namespace monitored_utils {

/**
 * @brief Create a monitored KD-tree with default settings
 * 
 * @tparam Distance Distance metric adaptor
 * @tparam DatasetAdaptor Dataset adaptor
 * @tparam DIM Dimension
 * @tparam index_t Index type
 * @param dimensionality Tree dimensionality
 * @param inputData Dataset adaptor
 * @param memory_threshold_mb Memory threshold in MB (default: 100MB)
 * @return MonitoredKDTreeIndex instance
 */
template <typename Distance, typename DatasetAdaptor, int DIM = -1, typename index_t = uint32_t>
MonitoredKDTreeIndex<Distance, DatasetAdaptor, DIM, index_t> create_monitored_index(
    const typename MonitoredKDTreeIndex<Distance, DatasetAdaptor, DIM, index_t>::Dimension dimensionality,
    const DatasetAdaptor& inputData,
    size_t memory_threshold_mb = 100) {
    
    return MonitoredKDTreeIndex<Distance, DatasetAdaptor, DIM, index_t>(
        dimensionality, inputData, memory_threshold_mb);
}

/**
 * @brief Create a monitored KD-tree with custom configuration
 * 
 * @tparam Distance Distance metric adaptor
 * @tparam DatasetAdaptor Dataset adaptor
 * @tparam DIM Dimension
 * @tparam index_t Index type
 * @param dimensionality Tree dimensionality
 * @param inputData Dataset adaptor
 * @param monitor_config Memory monitoring configuration
 * @param params Tree construction parameters
 * @return MonitoredKDTreeIndex instance
 */
template <typename Distance, typename DatasetAdaptor, int DIM = -1, typename index_t = uint32_t>
MonitoredKDTreeIndex<Distance, DatasetAdaptor, DIM, index_t> create_monitored_index(
    const typename MonitoredKDTreeIndex<Distance, DatasetAdaptor, DIM, index_t>::Dimension dimensionality,
    const DatasetAdaptor& inputData,
    const typename MonitoredKDTreeIndex<Distance, DatasetAdaptor, DIM, index_t>::MonitorConfig& monitor_config,
    const KDTreeSingleIndexAdaptorParams& params = {}) {
    
    return MonitoredKDTreeIndex<Distance, DatasetAdaptor, DIM, index_t>(
        dimensionality, inputData, params, monitor_config);
}

/**
 * @brief Estimate memory usage for tree construction and set appropriate threshold
 * 
 * @tparam Distance Distance metric adaptor
 * @tparam DatasetAdaptor Dataset adaptor
 * @tparam DIM Dimension
 * @tparam index_t Index type
 * @param dimensionality Tree dimensionality
 * @param inputData Dataset adaptor
 * @param safety_factor Safety factor for threshold (default: 2.0)
 * @return MonitoredKDTreeIndex instance with estimated threshold
 */
template <typename Distance, typename DatasetAdaptor, int DIM = -1, typename index_t = uint32_t>
MonitoredKDTreeIndex<Distance, DatasetAdaptor, DIM, index_t> create_smart_monitored_index(
    const typename MonitoredKDTreeIndex<Distance, DatasetAdaptor, DIM, index_t>::Dimension dimensionality,
    const DatasetAdaptor& inputData,
    double safety_factor = 2.0) {
    
    // Estimate memory usage
    size_t num_points = inputData.kdtree_get_point_count();
    size_t dimension = (DIM > 0) ? DIM : dimensionality;
    size_t estimated_memory = memory_utils::estimate_tree_memory_usage(
        num_points, dimension, sizeof(typename MonitoredKDTreeIndex<Distance, DatasetAdaptor, DIM, index_t>::ElementType));
    
    // Set threshold with safety factor
    size_t threshold = static_cast<size_t>(estimated_memory * safety_factor);
    
    return MonitoredKDTreeIndex<Distance, DatasetAdaptor, DIM, index_t>(
        dimensionality, inputData, threshold);
}

} // namespace monitored_utils

} // namespace nanoflann

#endif // NANOFLANN_MONITORED_INDEX_HPP
#ifndef NANOFLANN_MONITOR_WRAPPER_HPP
#define NANOFLANN_MONITOR_WRAPPER_HPP

#include "nanoflann.hpp"
#include "nanoflann_monitor.hpp"
#include <memory>
#include <string>

namespace NanoflannMonitor {

/**
 * A wrapper for nanoflann KDTreeSingleIndexAdaptor that automatically monitors memory usage
 * during tree construction and other memory-intensive operations.
 * 
 * This class inherits from nanoflann's KDTreeSingleIndexAdaptor and adds automatic
 * memory monitoring without requiring any changes to your existing code.
 * 
 * Usage:
 *   // Instead of:
 *   using my_kd_tree_t = nanoflann::KDTreeSingleIndexAdaptor<...>;
 *   
 *   // Use:
 *   using my_kd_tree_t = NanoflannMonitor::MonitoredKDTreeSingleIndexAdaptor<...>;
 */
template <typename Distance, typename DatasetAdaptor, int32_t DIM = -1, typename IndexType = uint32_t>
class MonitoredKDTreeSingleIndexAdaptor : public nanoflann::KDTreeSingleIndexAdaptor<Distance, DatasetAdaptor, DIM, IndexType> {
public:
    using BaseClass = nanoflann::KDTreeSingleIndexAdaptor<Distance, DatasetAdaptor, DIM, IndexType>;
    using Params = nanoflann::KDTreeSingleIndexAdaptorParams;
    
private:
    mutable MonitorConfig monitor_config_;
    mutable std::unique_ptr<MemoryMonitor> active_monitor_;
    std::string tree_name_;
    
    void start_monitoring(const std::string& operation) const {
        if (active_monitor_ && active_monitor_->is_monitoring()) {
            return; // Already monitoring
        }
        
        active_monitor_ = std::make_unique<MemoryMonitor>(monitor_config_);
        
        // Log operation start
        std::ostringstream oss;
        oss << "[NANOFLANN MONITOR] Starting " << operation;
        if (!tree_name_.empty()) {
            oss << " for tree '" << tree_name_ << "'";
        }
        
        if (monitor_config_.custom_logger) {
            monitor_config_.custom_logger(oss.str());
        } else if (monitor_config_.print_to_stderr) {
            std::cerr << oss.str() << std::endl;
        }
        
        active_monitor_->start();
    }
    
    void stop_monitoring(const std::string& operation) const {
        if (!active_monitor_ || !active_monitor_->is_monitoring()) {
            return;
        }
        
        auto peak_stats = active_monitor_->get_peak_stats();
        auto exceeded_count = active_monitor_->get_threshold_exceeded_count();
        
        active_monitor_->stop();
        
        // Log operation completion with summary
        std::ostringstream oss;
        oss << "[NANOFLANN MONITOR] Completed " << operation;
        if (!tree_name_.empty()) {
            oss << " for tree '" << tree_name_ << "'";
        }
        oss << "\n  Peak memory: " << std::fixed << std::setprecision(2)
            << (peak_stats.rss_bytes / (1024.0 * 1024.0)) << " MB";
        if (exceeded_count > 0) {
            oss << " (threshold exceeded " << exceeded_count << " times)";
        }
        
        if (monitor_config_.custom_logger) {
            monitor_config_.custom_logger(oss.str());
        } else if (monitor_config_.print_to_stderr) {
            std::cerr << oss.str() << std::endl;
        }
        
        active_monitor_.reset();
    }
    
public:
    /**
     * Constructor with monitoring configuration
     * @param dimensionality Dimensionality of the points
     * @param inputData Dataset adaptor
     * @param params KD-tree parameters
     * @param monitor_config Memory monitoring configuration
     * @param tree_name Optional name for this tree (used in logging)
     */
    MonitoredKDTreeSingleIndexAdaptor(
        const int dimensionality,
        const DatasetAdaptor& inputData,
        const Params& params = Params(),
        const MonitorConfig& monitor_config = MonitorConfig{},
        const std::string& tree_name = "")
        : BaseClass(dimensionality, inputData, params)
        , monitor_config_(monitor_config)
        , tree_name_(tree_name) {
    }
    
    /**
     * Build the KD-tree index with automatic memory monitoring
     */
    void buildIndex() {
        start_monitoring("buildIndex()");
        
        try {
            BaseClass::buildIndex();
            stop_monitoring("buildIndex()");
        } catch (...) {
            stop_monitoring("buildIndex() (failed)");
            throw;
        }
    }
    
    /**
     * Set the memory monitoring configuration
     */
    void setMonitorConfig(const MonitorConfig& config) {
        monitor_config_ = config;
    }
    
    /**
     * Get the current monitoring configuration
     */
    const MonitorConfig& getMonitorConfig() const {
        return monitor_config_;
    }
    
    /**
     * Set a name for this tree (used in logging)
     */
    void setTreeName(const std::string& name) {
        tree_name_ = name;
    }
    
    /**
     * Get the tree name
     */
    const std::string& getTreeName() const {
        return tree_name_;
    }
    
    /**
     * Enable or disable monitoring
     */
    void setMonitoringEnabled(bool enabled) {
        monitor_config_.print_to_stderr = enabled;
    }
};

/**
 * A wrapper for nanoflann KDTreeSingleIndexDynamicAdaptor that automatically monitors memory usage
 * This is for the dynamic version that supports adding/removing points
 */
template <typename Distance, typename DatasetAdaptor, int32_t DIM = -1, typename IndexType = uint32_t>
class MonitoredKDTreeDynamicAdaptor : public nanoflann::KDTreeSingleIndexDynamicAdaptor<Distance, DatasetAdaptor, DIM, IndexType> {
public:
    using BaseClass = nanoflann::KDTreeSingleIndexDynamicAdaptor<Distance, DatasetAdaptor, DIM, IndexType>;
    using Params = nanoflann::KDTreeSingleIndexAdaptorParams;
    
private:
    mutable MonitorConfig monitor_config_;
    mutable std::unique_ptr<MemoryMonitor> active_monitor_;
    std::string tree_name_;
    
    void start_monitoring(const std::string& operation) const {
        if (active_monitor_ && active_monitor_->is_monitoring()) {
            return;
        }
        
        active_monitor_ = std::make_unique<MemoryMonitor>(monitor_config_);
        active_monitor_->start();
    }
    
    void stop_monitoring(const std::string& operation) const {
        if (!active_monitor_ || !active_monitor_->is_monitoring()) {
            return;
        }
        
        auto exceeded_count = active_monitor_->get_threshold_exceeded_count();
        if (exceeded_count > 0) {
            auto peak_stats = active_monitor_->get_peak_stats();
            
            std::ostringstream oss;
            oss << "[NANOFLANN MONITOR] " << operation;
            if (!tree_name_.empty()) {
                oss << " for tree '" << tree_name_ << "'";
            }
            oss << " - Peak memory: " << std::fixed << std::setprecision(2)
                << (peak_stats.rss_bytes / (1024.0 * 1024.0)) << " MB"
                << " (threshold exceeded)";
            
            if (monitor_config_.custom_logger) {
                monitor_config_.custom_logger(oss.str());
            } else if (monitor_config_.print_to_stderr) {
                std::cerr << oss.str() << std::endl;
            }
        }
        
        active_monitor_->stop();
        active_monitor_.reset();
    }
    
public:
    MonitoredKDTreeDynamicAdaptor(
        const int dimensionality,
        const DatasetAdaptor& inputData,
        const Params& params = Params(),
        const size_t maximumPointCount = 1000000000U,
        const MonitorConfig& monitor_config = MonitorConfig{},
        const std::string& tree_name = "")
        : BaseClass(dimensionality, inputData, params, maximumPointCount)
        , monitor_config_(monitor_config)
        , tree_name_(tree_name) {
        // Note: The dynamic adaptor will automatically add initial points if dataset is not empty
    }
    
    void addPoints(IndexType start, IndexType end) {
        size_t num_points = end - start;
        if (num_points > 10000) {  // Only monitor large additions
            start_monitoring("addPoints()");
            
            try {
                BaseClass::addPoints(start, end);
                stop_monitoring("addPoints()");
            } catch (...) {
                stop_monitoring("addPoints() (failed)");
                throw;
            }
        } else {
            BaseClass::addPoints(start, end);
        }
    }
    
    void removePoint(IndexType idx) {
        BaseClass::removePoint(idx);
    }
    
    void setMonitorConfig(const MonitorConfig& config) {
        monitor_config_ = config;
    }
    
    const MonitorConfig& getMonitorConfig() const {
        return monitor_config_;
    }
    
    void setTreeName(const std::string& name) {
        tree_name_ = name;
    }
    
    const std::string& getTreeName() const {
        return tree_name_;
    }
};

/**
 * Helper function to create a monitored KD-tree with a fluent interface
 */
template <typename Distance, typename DatasetAdaptor, int32_t DIM = -1, typename IndexType = uint32_t>
auto makeMonitoredKDTree(
    const int dimensionality,
    const DatasetAdaptor& dataset,
    size_t threshold_mb = 100,
    const std::string& tree_name = "") {
    
    using TreeType = MonitoredKDTreeSingleIndexAdaptor<Distance, DatasetAdaptor, DIM, IndexType>;
    
    MonitorConfig config;
    config.threshold_mb = threshold_mb;
    config.check_interval_ms = 50;  // More frequent checks for accuracy
    
    nanoflann::KDTreeSingleIndexAdaptorParams params(10);
    
    return TreeType(
        dimensionality,
        dataset,
        params,
        config,
        tree_name
    );
}

} // namespace NanoflannMonitor

#endif // NANOFLANN_MONITOR_WRAPPER_HPP
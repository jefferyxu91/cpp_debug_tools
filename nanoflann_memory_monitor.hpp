/**
 * Memory-monitored nanoflann extension
 * 
 * This header provides a derived class that extends nanoflann's KDTreeSingleIndexAdaptor
 * with memory monitoring capabilities. The build index process will be interrupted
 * when memory usage exceeds a specified threshold.
 * 
 * Usage:
 *   #include "nanoflann_memory_monitor.hpp"
 *   
 *   // Create with memory threshold (in bytes)
 *   MemoryMonitoredKDTree<Distance, DatasetAdaptor, DIM, IndexType> 
 *       index(dim, dataset, params, memory_threshold_bytes);
 *   
 *   // Build index with memory monitoring
 *   try {
 *       index.buildIndex();
 *   } catch (const MemoryLimitExceededException& e) {
 *       // Handle memory limit exceeded
 *   }
 */

#pragma once

#include "nanoflann/include/nanoflann.hpp"
#include <sys/resource.h>
#include <sys/time.h>
#include <stdexcept>
#include <functional>
#include <cstring>

namespace nanoflann {

// Forward declaration of assign function
template <typename Container, typename T>
inline typename std::enable_if<has_assign<Container>::value, void>::type assign(
    Container& c, const size_t nElements, const T& value);

template <typename Container, typename T>
inline typename std::enable_if<!has_assign<Container>::value, void>::type
    assign(Container& c, const size_t nElements, const T& value);

/**
 * Exception thrown when memory limit is exceeded during index building
 */
class MemoryLimitExceededException : public std::runtime_error {
public:
    explicit MemoryLimitExceededException(const std::string& message) 
        : std::runtime_error(message) {}
};

/**
 * Memory monitoring utility class
 */
class MemoryMonitor {
private:
    size_t memory_threshold_;
    std::function<size_t()> memory_checker_;
    
public:
    explicit MemoryMonitor(size_t memory_threshold_bytes) 
        : memory_threshold_(memory_threshold_bytes) {
        
        // Set up memory checking function based on available methods
        #ifdef __linux__
            memory_checker_ = [this]() { return getCurrentMemoryUsageLinux(); };
        #else
            memory_checker_ = [this]() { return getCurrentMemoryUsageFallback(); };
        #endif
    }
    
    /**
     * Check if current memory usage exceeds threshold
     * @return true if memory limit exceeded
     */
    bool checkMemoryLimit() const {
        return memory_checker_() > memory_threshold_;
    }
    
    /**
     * Get current memory usage in bytes
     * @return memory usage in bytes
     */
    size_t getCurrentMemoryUsage() const {
        return memory_checker_();
    }
    
    /**
     * Get memory threshold in bytes
     * @return memory threshold
     */
    size_t getMemoryThreshold() const {
        return memory_threshold_;
    }

private:
    /**
     * Get current memory usage on Linux systems using /proc/self/status
     * @return memory usage in bytes
     */
    size_t getCurrentMemoryUsageLinux() const {
        #ifdef __linux__
        FILE* file = fopen("/proc/self/status", "r");
        if (file) {
            char line[128];
            size_t vm_rss = 0;
            
            while (fgets(line, 128, file) != nullptr) {
                if (strncmp(line, "VmRSS:", 6) == 0) {
                    sscanf(line, "VmRSS: %zu", &vm_rss);
                    break;
                }
            }
            fclose(file);
            return vm_rss * 1024; // Convert KB to bytes
        }
        #endif
        
        return getCurrentMemoryUsageFallback();
    }
    
    /**
     * Fallback memory usage method using getrusage
     * @return memory usage in bytes
     */
    size_t getCurrentMemoryUsageFallback() const {
        struct rusage rusage;
        if (getrusage(RUSAGE_SELF, &rusage) == 0) {
            return static_cast<size_t>(rusage.ru_maxrss) * 1024; // Convert KB to bytes
        }
        return 0; // Unable to determine memory usage
    }
};

/**
 * Memory-monitored pooled allocator that checks memory limits
 */
template<typename BaseAllocator = PooledAllocator>
class MemoryMonitoredAllocator : public BaseAllocator {
private:
    MemoryMonitor* memory_monitor_;
    
public:
    explicit MemoryMonitoredAllocator(MemoryMonitor* monitor = nullptr) 
        : BaseAllocator(), memory_monitor_(monitor) {}
    
    /**
     * Override malloc to check memory limits
     */
    void* malloc(const size_t req_size) {
        if (memory_monitor_ && memory_monitor_->checkMemoryLimit()) {
            throw MemoryLimitExceededException(
                "Memory limit exceeded during allocation. "
                "Current: " + std::to_string(memory_monitor_->getCurrentMemoryUsage()) + 
                " bytes, Threshold: " + std::to_string(memory_monitor_->getMemoryThreshold()) + " bytes");
        }
        return BaseAllocator::malloc(req_size);
    }
    
    /**
     * Set memory monitor
     */
    void setMemoryMonitor(MemoryMonitor* monitor) {
        memory_monitor_ = monitor;
    }
};

/**
 * Memory-monitored KD-tree base class
 */
template<typename Derived, typename Distance, typename DatasetAdaptor, 
         int32_t DIM = -1, typename IndexType = uint32_t>
class MemoryMonitoredKDTreeBase 
    : public KDTreeBaseClass<Derived, Distance, DatasetAdaptor, DIM, IndexType> {
    
protected:
    MemoryMonitor memory_monitor_;
    MemoryMonitoredAllocator<> monitored_pool_;
    
public:
    using Base = KDTreeBaseClass<Derived, Distance, DatasetAdaptor, DIM, IndexType>;
    using typename Base::NodePtr;
    using typename Base::Offset;
    using typename Base::Size;
    using typename Base::Dimension;
    using typename Base::BoundingBox;
    using typename Base::ElementType;
    using typename Base::DistanceType;
    
    explicit MemoryMonitoredKDTreeBase(size_t memory_threshold_bytes)
        : memory_monitor_(memory_threshold_bytes) {
        monitored_pool_.setMemoryMonitor(&memory_monitor_);
    }
    
    /**
     * Override divideTree to use monitored allocator
     */
    NodePtr divideTree(Derived& obj, const Offset left, const Offset right, BoundingBox& bbox) {
        // Check memory before allocation
        if (memory_monitor_.checkMemoryLimit()) {
            throw MemoryLimitExceededException(
                "Memory limit exceeded during tree division. "
                "Current: " + std::to_string(memory_monitor_.getCurrentMemoryUsage()) + 
                " bytes, Threshold: " + std::to_string(memory_monitor_.getMemoryThreshold()) + " bytes");
        }
        
        // Use monitored allocator instead of base allocator
        NodePtr node = monitored_pool_.template allocate<typename Base::Node>();
        const auto dims = (DIM > 0 ? DIM : obj.dim_);

        /* If too few exemplars remain, then make this a leaf node. */
        if ((right - left) <= static_cast<Offset>(obj.leaf_max_size_)) {
            node->child1 = node->child2 = nullptr; /* Mark as leaf node. */
            node->node_type.lr.left     = left;
            node->node_type.lr.right    = right;

            // compute bounding-box of leaf points
            for (Dimension i = 0; i < dims; ++i) {
                bbox[i].low  = Base::dataset_get(obj, obj.vAcc_[left], i);
                bbox[i].high = Base::dataset_get(obj, obj.vAcc_[left], i);
            }
            for (Offset k = left + 1; k < right; ++k) {
                for (Dimension i = 0; i < dims; ++i) {
                    const auto val = Base::dataset_get(obj, obj.vAcc_[k], i);
                    if (bbox[i].low > val) bbox[i].low = val;
                    if (bbox[i].high < val) bbox[i].high = val;
                }
            }
        } else {
            Offset       idx;
            Dimension    cutfeat;
            typename Base::DistanceType cutval;
            Base::middleSplit_(obj, left, right - left, idx, cutfeat, cutval, bbox);

            node->node_type.sub.divfeat = cutfeat;

            BoundingBox left_bbox(bbox);
            left_bbox[cutfeat].high = cutval;
            node->child1 = this->divideTree(obj, left, left + idx, left_bbox);

            BoundingBox right_bbox(bbox);
            right_bbox[cutfeat].low = cutval;
            node->child2 = this->divideTree(obj, left + idx, right, right_bbox);

            node->node_type.sub.divlow  = left_bbox[cutfeat].high;
            node->node_type.sub.divhigh = right_bbox[cutfeat].low;

            for (Dimension i = 0; i < dims; ++i) {
                bbox[i].low  = std::min(left_bbox[i].low, right_bbox[i].low);
                bbox[i].high = std::max(left_bbox[i].high, right_bbox[i].high);
            }
        }

        return node;
    }
    
    /**
     * Get memory monitor
     */
    const MemoryMonitor& getMemoryMonitor() const {
        return memory_monitor_;
    }
    
    /**
     * Get current memory usage
     */
    size_t getCurrentMemoryUsage() const {
        return memory_monitor_.getCurrentMemoryUsage();
    }
    
    /**
     * Get memory threshold
     */
    size_t getMemoryThreshold() const {
        return memory_monitor_.getMemoryThreshold();
    }
    
    /**
     * Search level implementation
     */
    template <class RESULTSET>
    bool searchLevel(
        RESULTSET& result_set, const ElementType* vec, const NodePtr node,
        DistanceType mindist, typename Base::distance_vector_t& dists,
        const float epsError) const {
        /* If this is a leaf node, then do check and return. */
        if ((node->child1 == nullptr) && (node->child2 == nullptr)) {
            DistanceType worst_dist = result_set.worstDist();
            for (Offset i = node->node_type.lr.left;
                 i < node->node_type.lr.right; ++i) {
                const IndexType accessor = Base::vAcc_[i];
                DistanceType    dist     = static_cast<const Derived*>(this)->distance_.evalMetric(
                           vec, accessor, (DIM > 0 ? DIM : Base::dim_));
                if (dist < worst_dist) {
                    if (!result_set.addPoint(dist, Base::vAcc_[i])) {
                        // the resultset doesn't want to receive any more
                        // points, we're done searching!
                        return false;
                    }
                }
            }
            return true;
        }

        /* Which child branch should be taken first? */
        Dimension    idx   = node->node_type.sub.divfeat;
        ElementType  val   = vec[idx];
        DistanceType diff1 = val - node->node_type.sub.divlow;
        DistanceType diff2 = val - node->node_type.sub.divhigh;

        NodePtr      bestChild;
        NodePtr      otherChild;
        DistanceType cut_dist;
        if ((diff1 + diff2) < 0) {
            bestChild  = node->child1;
            otherChild = node->child2;
            cut_dist =
                static_cast<const Derived*>(this)->distance_.accum_dist(val, node->node_type.sub.divhigh, idx);
        } else {
            bestChild  = node->child2;
            otherChild = node->child1;
            cut_dist =
                static_cast<const Derived*>(this)->distance_.accum_dist(val, node->node_type.sub.divlow, idx);
        }

        /* Call recursively to search next level down. */
        if (!searchLevel(result_set, vec, bestChild, mindist, dists, epsError)) {
            // the resultset doesn't want to receive any more points, we're done
            // searching!
            return false;
        }

        DistanceType dst = dists[idx];
        mindist          = mindist + cut_dist - dst;
        dists[idx]       = cut_dist;
        if (mindist * epsError <= result_set.worstDist()) {
            if (!searchLevel(
                    result_set, vec, otherChild, mindist, dists, epsError)) {
                // the resultset doesn't want to receive any more points, we're
                // done searching!
                return false;
            }
        }
        dists[idx] = dst;
        return true;
    }
    
    /**
     * Compute initial distances
     */
    DistanceType computeInitialDistances(
        const Derived& obj, const ElementType* vec,
        typename Base::distance_vector_t& dists) const {
        assert(vec);
        DistanceType dist = DistanceType();

        for (Dimension i = 0; i < (DIM > 0 ? DIM : obj.dim_); ++i) {
            if (vec[i] < obj.root_bbox_[i].low) {
                            dists[i] =
                static_cast<const Derived*>(this)->distance_.accum_dist(vec[i], obj.root_bbox_[i].low, i);
                dist += dists[i];
            }
            if (vec[i] > obj.root_bbox_[i].high) {
                            dists[i] =
                static_cast<const Derived*>(this)->distance_.accum_dist(vec[i], obj.root_bbox_[i].high, i);
                dist += dists[i];
            }
        }
        return dist;
    }
};

/**
 * Memory-monitored KD-tree single index adaptor
 */
template<typename Distance, typename DatasetAdaptor, int32_t DIM = -1, typename IndexType = uint32_t>
class MemoryMonitoredKDTree 
    : public MemoryMonitoredKDTreeBase<
          MemoryMonitoredKDTree<Distance, DatasetAdaptor, DIM, IndexType>,
          Distance, DatasetAdaptor, DIM, IndexType> {
    
public:
    using Base = MemoryMonitoredKDTreeBase<
        MemoryMonitoredKDTree<Distance, DatasetAdaptor, DIM, IndexType>,
        Distance, DatasetAdaptor, DIM, IndexType>;
    using typename Base::ElementType;
    using typename Base::DistanceType;
    using typename Base::Dimension;
    using typename Base::Size;
    using typename Base::Offset;
    
    const DatasetAdaptor dataset_;
    const KDTreeSingleIndexAdaptorParams indexParams;
    Distance distance_;
    
    /**
     * Constructor with memory threshold
     */
    explicit MemoryMonitoredKDTree(
        const Dimension dimensionality, 
        const DatasetAdaptor& inputData,
        const KDTreeSingleIndexAdaptorParams& params,
        size_t memory_threshold_bytes)
        : Base(memory_threshold_bytes),
          dataset_(inputData),
          indexParams(params),
          distance_(inputData) {
        
        init(dimensionality);
        
        if (!(params.flags & KDTreeSingleIndexAdaptorFlags::SkipInitialBuildIndex)) {
            buildIndex();
        }
    }
    
    /**
     * Constructor with default parameters
     */
    explicit MemoryMonitoredKDTree(
        const Dimension dimensionality, 
        const DatasetAdaptor& inputData,
        size_t memory_threshold_bytes,
        const KDTreeSingleIndexAdaptorParams& params = {})
        : MemoryMonitoredKDTree(dimensionality, inputData, params, memory_threshold_bytes) {
        (void)dimensionality; // Suppress unused parameter warning
    }
    
private:
    void init(const Dimension dimensionality) {
        Base::dim_ = dimensionality;
        Base::leaf_max_size_ = indexParams.leaf_max_size;
        Base::n_thread_build_ = indexParams.n_thread_build;
        
        if (Base::dim_ == 0) {
            throw std::runtime_error("Error: dimensionality cannot be zero");
        }
        
        Base::size_ = dataset_.kdtree_get_point_count();
        if (Base::size_ == 0) {
            throw std::runtime_error("Error: dataset is empty");
        }
        
        init_vind();
        computeBoundingBox(Base::root_bbox_);
    }
    
public:
    /**
     * Build index with memory monitoring
     */
    void buildIndex() {
        Base::size_ = dataset_.kdtree_get_point_count();
        Base::size_at_index_build_ = Base::size_;
        init_vind();
        Base::freeIndex(*this);
        Base::size_at_index_build_ = Base::size_;
        
        if (Base::size_ == 0) return;
        
        computeBoundingBox(Base::root_bbox_);
        
        // Check memory before starting tree construction
        if (Base::getMemoryMonitor().checkMemoryLimit()) {
            throw MemoryLimitExceededException(
                "Memory limit exceeded before tree construction. "
                "Current: " + std::to_string(Base::getCurrentMemoryUsage()) + 
                " bytes, Threshold: " + std::to_string(Base::getMemoryThreshold()) + " bytes");
        }
        
        // construct the tree
        if (Base::n_thread_build_ == 1) {
            Base::root_node_ = Base::divideTree(*this, 0, Base::size_, Base::root_bbox_);
        } else {
            #ifndef NANOFLANN_NO_THREADS
                std::atomic<unsigned int> thread_count(0u);
                std::mutex mutex;
                Base::root_node_ = Base::divideTreeConcurrent(
                    *this, 0, Base::size_, Base::root_bbox_, thread_count, mutex);
            #else
                throw std::runtime_error("Multithreading is disabled");
            #endif
        }
    }
    
    // Implement search methods
    template <typename RESULTSET>
    bool findNeighbors(
        RESULTSET& result, const ElementType* vec,
        const SearchParameters& searchParams = {}) const {
        assert(vec);
        if (Base::size(*this) == 0) return false;
        if (!Base::root_node_)
            throw std::runtime_error(
                "[nanoflann] findNeighbors() called before building the index.");
        float epsError = 1 + searchParams.eps;

        // fixed or variable-sized container (depending on DIM)
        typename Base::distance_vector_t dists;
        // Fill it with zeros.
        auto zero = static_cast<decltype(result.worstDist())>(0);
        assign(dists, (DIM > 0 ? DIM : Base::dim_), zero);
        DistanceType dist = Base::computeInitialDistances(*this, vec, dists);
        Base::searchLevel(result, vec, Base::root_node_, dist, dists, epsError);

        if (searchParams.sorted) result.sort();

        return result.full();
    }

    Size knnSearch(
        const ElementType* query_point, const Size num_closest,
        IndexType* out_indices, DistanceType* out_distances) const {
        nanoflann::KNNResultSet<DistanceType, IndexType> resultSet(num_closest);
        resultSet.init(out_indices, out_distances);
        findNeighbors(resultSet, query_point);
        return resultSet.size();
    }

    Size radiusSearch(
        const ElementType* query_point, const DistanceType radius,
        std::vector<std::pair<IndexType, DistanceType>>& IndicesDists) const {
        nanoflann::RadiusResultSet<DistanceType, IndexType> resultSet(radius, IndicesDists);
        findNeighbors(resultSet, query_point);
        return resultSet.size();
    }
    
    // Forward the required interface methods
    Size kdtree_get_point_count() const { return dataset_.kdtree_get_point_count(); }
    ElementType kdtree_get_pt(const IndexType idx, size_t dim) const { return dataset_.kdtree_get_pt(idx, dim); }
    
    template <class BBOX>
    bool kdtree_get_bbox(BBOX& bb) const { return dataset_.kdtree_get_bbox(bb); }
    
    void init_vind() {
        Base::vAcc_.resize(Base::size_);
        for (Size i = 0; i < Base::size_; ++i) {
            Base::vAcc_[i] = i;
        }
    }
    
    void computeBoundingBox(typename Base::BoundingBox& bbox) {
        const auto dims = (DIM > 0 ? DIM : Base::dim_);
        for (Dimension i = 0; i < dims; ++i) {
            bbox[i].low = bbox[i].high = dataset_.kdtree_get_pt(Base::vAcc_[0], i);
        }
        for (Size k = 1; k < Base::size_; ++k) {
            for (Dimension i = 0; i < dims; ++i) {
                const auto val = dataset_.kdtree_get_pt(Base::vAcc_[k], i);
                if (bbox[i].low > val) bbox[i].low = val;
                if (bbox[i].high < val) bbox[i].high = val;
            }
        }
    }
};

} // namespace nanoflann
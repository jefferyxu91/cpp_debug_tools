/*
 * nanoflann_kdtree_limited.hpp
 * ----------------------------------
 * A drop-in replacement for nanoflann::KDTreeSingleIndexAdaptor that aborts the
 * build the very moment the memory budget is exceeded.  It clones the exact
 * build algorithm of nanoflann 1.6.1 (non-concurrent version) and injects a
 * test at the start of every recursive divideTree() call.  If the process RSS
 * Δ or the internal pooled-allocator Δ exceeds the limits supplied by the user
 * a MemoryExceeded exception is thrown.
 */

#ifndef NANOFLANN_KDTREE_LIMITED_HPP
#define NANOFLANN_KDTREE_LIMITED_HPP

#include <nanoflann.hpp>
#include <memory/nanoflann_memory_watcher.hpp>
#include <stdexcept>
#include <unistd.h>
#include <fstream>

namespace nanoflann_limited {

// Utility: read current RSS in bytes (Linux-/proc based)
inline std::size_t currentRSS()
{
    std::ifstream statm("/proc/self/statm");
    if (!statm.is_open()) return 0;
    long pages = 0, resident = 0; statm >> pages >> resident;
    return std::size_t(resident) * std::size_t(sysconf(_SC_PAGESIZE));
}

struct MemoryExceeded : public std::runtime_error
{
    explicit MemoryExceeded(const char* msg) : std::runtime_error(msg) {}
};

// The template mirrors KDTreeSingleIndexAdaptor
template <class Distance, class DatasetAdaptor, int DIM = -1, typename IndexType = size_t>
class KDTreeSingleIndexAdaptorLimited
    : public nanoflann::KDTreeSingleIndexAdaptor<Distance, DatasetAdaptor, DIM, IndexType>
{
    using Base = nanoflann::KDTreeSingleIndexAdaptor<Distance, DatasetAdaptor, DIM, IndexType>;
    using BaseBase = typename Base::Base; // KDTreeBaseClass
    using NodePtr = typename BaseBase::NodePtr;
    using BoundingBox = typename BaseBase::BoundingBox;
    using Offset = typename BaseBase::Offset;
    using Size = typename BaseBase::Size;
    using Dimension = typename BaseBase::Dimension;
    using DistanceType = typename BaseBase::DistanceType;
    using ElementType = typename BaseBase::ElementType;

public:
    template<typename... Args>
    KDTreeSingleIndexAdaptorLimited(std::size_t rssLimitBytes,
                                    std::size_t poolLimitBytes,
                                    Args&&... args)
        : Base(std::forward<Args>(args)...),
          rss_limit_(rssLimitBytes), pool_limit_(poolLimitBytes)
    {}

    // Re-implemented buildIndex (non-threaded path only)
    void buildIndex()
    {
        baseline_rss_ = currentRSS();
        auto& obj = *this;
        BaseBase::size_ = this->dataset_.kdtree_get_point_count();
        BaseBase::size_at_index_build_ = BaseBase::size_;
        this->init_vind();
        this->freeIndex(*this);
        BaseBase::size_at_index_build_ = BaseBase::size_;
        if (BaseBase::size_ == 0) return;
        this->computeBoundingBox(BaseBase::root_bbox_);
        try {
            BaseBase::root_node_ = divideTreeLimited(*this, 0, BaseBase::size_, BaseBase::root_bbox_);
        } catch(...) {
            // ensure we leave the object in a consistent state (empty tree)
            this->freeIndex(*this);
            throw;
        }
    }

private:
    bool exceeded(const BaseBase& obj) const
    {
        const std::size_t rss_now = currentRSS();
        if (rss_limit_ && (rss_now > baseline_rss_) && (rss_now - baseline_rss_ > rss_limit_)) return true;
        if (pool_limit_ && (obj.pool_.usedMemory + obj.pool_.wastedMemory > pool_limit_)) return true;
        return false;
    }

    NodePtr divideTreeLimited(BaseBase& obj, const Offset left, const Offset right, BoundingBox& bbox)
    {
        if (exceeded(obj)) throw MemoryExceeded("KD-tree build: memory limit exceeded");

        const auto dims = (DIM > 0 ? DIM : obj.dim_);
        NodePtr node = obj.pool_.template allocate<typename BaseBase::Node>();

        if ((right - left) <= static_cast<Offset>(obj.leaf_max_size_))
        {
            node->child1 = node->child2 = nullptr;
            node->node_type.lr.left = left;
            node->node_type.lr.right = right;
            for (Dimension i = 0; i < dims; ++i)
            {
                bbox[i].low  = obj.dataset_get(obj, obj.vAcc_[left], i);
                bbox[i].high = bbox[i].low;
            }
            for (Offset k = left + 1; k < right; ++k)
            {
                for (Dimension i = 0; i < dims; ++i)
                {
                    const auto val = obj.dataset_get(obj, obj.vAcc_[k], i);
                    if (bbox[i].low > val) bbox[i].low = val;
                    if (bbox[i].high < val) bbox[i].high = val;
                }
            }
        }
        else
        {
            Offset idx; Dimension cutfeat; DistanceType cutval;
            this->middleSplit_(obj, left, right - left, idx, cutfeat, cutval, bbox);
            node->node_type.sub.divfeat = cutfeat;

            BoundingBox left_bbox(bbox);  left_bbox[cutfeat].high = cutval;
            node->child1 = divideTreeLimited(obj, left, left + idx, left_bbox);

            BoundingBox right_bbox(bbox); right_bbox[cutfeat].low = cutval;
            node->child2 = divideTreeLimited(obj, left + idx, right, right_bbox);

            node->node_type.sub.divlow  = left_bbox[cutfeat].high;
            node->node_type.sub.divhigh = right_bbox[cutfeat].low;

            for (Dimension i = 0; i < dims; ++i)
            {
                bbox[i].low  = std::min(left_bbox[i].low,  right_bbox[i].low);
                bbox[i].high = std::max(left_bbox[i].high, right_bbox[i].high);
            }
        }
        return node;
    }

    std::size_t rss_limit_;
    std::size_t pool_limit_;
    std::size_t baseline_rss_{0};
};

} // namespace nanoflann_limited

#endif // NANOFLANN_KDTREE_LIMITED_HPP
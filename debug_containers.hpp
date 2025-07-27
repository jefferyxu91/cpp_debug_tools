#ifndef DEBUG_CONTAINERS_HPP
#define DEBUG_CONTAINERS_HPP

#include <iostream>
#include <vector>
#include <list>
#include <deque>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
#include <string>
#include <memory>
#include <cstddef>

namespace Debug {

// Configuration
constexpr size_t DEFAULT_MEMORY_THRESHOLD = 20 * 1024 * 1024; // 20MB

// Global threshold setting
inline size_t memory_threshold = DEFAULT_MEMORY_THRESHOLD;

// Utility function to print allocation info
inline void print_allocation_info(size_t size, const char* file, int line) {
    if (size > memory_threshold) {
        std::cout << "[DEBUG] Large allocation detected: " << size << " bytes at " 
                  << file << ":" << line << std::endl;
    }
}

// Custom allocator that tracks allocations
template<typename T>
class DebugAllocator {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    DebugAllocator() = default;
    template<typename U>
    DebugAllocator(const DebugAllocator<U>&) noexcept {}

    T* allocate(size_type n) {
        size_t total_size = n * sizeof(T);
        print_allocation_info(total_size, __FILE__, __LINE__);
        return std::allocator<T>{}.allocate(n);
    }

    void deallocate(T* p, size_type n) noexcept {
        std::allocator<T>{}.deallocate(p, n);
    }

    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        std::allocator<T>{}.construct(p, std::forward<Args>(args)...);
    }

    template<typename U>
    void destroy(U* p) {
        std::allocator<T>{}.destroy(p);
    }
};

template<typename T, typename U>
bool operator==(const DebugAllocator<T>&, const DebugAllocator<U>&) noexcept {
    return true;
}

template<typename T, typename U>
bool operator!=(const DebugAllocator<T>& lhs, const DebugAllocator<U>& rhs) noexcept {
    return !(lhs == rhs);
}

// Container wrappers
template<typename T>
using vector = std::vector<T, DebugAllocator<T>>;

template<typename T>
using list = std::list<T, DebugAllocator<T>>;

template<typename T>
using deque = std::deque<T, DebugAllocator<T>>;

template<typename T>
using set = std::set<T, std::less<T>, DebugAllocator<T>>;

template<typename T>
using multiset = std::multiset<T, std::less<T>, DebugAllocator<T>>;

template<typename Key, typename Value>
using map = std::map<Key, Value, std::less<Key>, DebugAllocator<std::pair<const Key, Value>>>;

template<typename Key, typename Value>
using multimap = std::multimap<Key, Value, std::less<Key>, DebugAllocator<std::pair<const Key, Value>>>;

template<typename T>
using unordered_set = std::unordered_set<T, std::hash<T>, std::equal_to<T>, DebugAllocator<T>>;

template<typename T>
using unordered_multiset = std::unordered_multiset<T, std::hash<T>, std::equal_to<T>, DebugAllocator<T>>;

template<typename Key, typename Value>
using unordered_map = std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>, 
                                        DebugAllocator<std::pair<const Key, Value>>>;

template<typename Key, typename Value>
using unordered_multimap = std::unordered_multimap<Key, Value, std::hash<Key>, std::equal_to<Key>, 
                                                  DebugAllocator<std::pair<const Key, Value>>>;

template<typename T>
using stack = std::stack<T, deque<T>>;

template<typename T>
using queue = std::queue<T, deque<T>>;

template<typename T>
using priority_queue = std::priority_queue<T, vector<T>>;

// String wrapper
using string = std::basic_string<char, std::char_traits<char>, DebugAllocator<char>>;

// Function to set memory threshold
inline void set_memory_threshold(size_t threshold) {
    memory_threshold = threshold;
}

// Function to get current memory threshold
inline size_t get_memory_threshold() {
    return memory_threshold;
}

} // namespace Debug

#endif // DEBUG_CONTAINERS_HPP
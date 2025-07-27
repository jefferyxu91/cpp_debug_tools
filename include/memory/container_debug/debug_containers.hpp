#ifndef SIMPLE_DEBUG_CONTAINERS_HPP
#define SIMPLE_DEBUG_CONTAINERS_HPP

#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <deque>
#include <queue>
#include <stack>
#include <functional>

namespace Debug {

// Configuration
constexpr size_t DEFAULT_MEMORY_THRESHOLD = 20 * 1024 * 1024; // 20MB
inline size_t memory_threshold = DEFAULT_MEMORY_THRESHOLD;

// Global output stream function - defaults to std::cerr
inline std::function<void(const std::string&)> output_stream = [](const std::string& message) {
    std::cerr << message << std::endl;
};

// Function to set custom output stream
inline void set_output_stream(std::function<void(const std::string&)> stream) {
    output_stream = stream;
}

// Function to set memory threshold
inline void set_memory_threshold(size_t threshold) {
    memory_threshold = threshold;
}

// Function to get current memory threshold
inline size_t get_memory_threshold() {
    return memory_threshold;
}

// Simple allocator that tracks allocations
template<typename T>
class SimpleAllocator {
public:
    using value_type = T;
    
    template<typename U>
    struct rebind {
        using other = SimpleAllocator<U>;
    };

    T* allocate(size_t n) {
        size_t bytes = n * sizeof(T);
        
        if (bytes > memory_threshold) {
            std::string message = "[DEBUG] Large allocation detected: " + 
                                 std::to_string(bytes) + " bytes (" + 
                                 std::to_string(bytes / (1024.0 * 1024.0)) + " MB)";
            output_stream(message);
        }
        
        return static_cast<T*>(std::malloc(bytes));
    }

    void deallocate(T* ptr, size_t) noexcept {
        std::free(ptr);
    }
};

// Macro for container allocations - automatically captures file and line
#define DEBUG_ALLOC(size) \
    do { \
        if ((size) > Debug::memory_threshold) { \
            std::string message = "[DEBUG] Large allocation detected: " + \
                                 std::to_string(size) + " bytes (" + \
                                 std::to_string((size) / (1024.0 * 1024.0)) + " MB) at " + \
                                 std::string(__FILE__) + ":" + std::to_string(__LINE__) + \
                                 " in function '" + std::string(__FUNCTION__) + "'"; \
            Debug::output_stream(message); \
        } \
    } while(0)

// Vector
template<typename T, typename Alloc = SimpleAllocator<T>>
class vector : public std::vector<T, Alloc> {
public:
    using base_type = std::vector<T, Alloc>;
    
    vector() : base_type() {}
    vector(size_t count) : base_type(count) {
        DEBUG_ALLOC(count * sizeof(T));
    }
    vector(size_t count, const T& value) : base_type(count, value) {
        DEBUG_ALLOC(count * sizeof(T));
    }
    vector(const vector& other) : base_type(other) {}
    vector(vector&& other) noexcept : base_type(std::move(other)) {}
    
    vector& operator=(const vector& other) {
        base_type::operator=(other);
        return *this;
    }
    
    vector& operator=(vector&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
    
    void resize(size_t count) {
        DEBUG_ALLOC(count * sizeof(T));
        base_type::resize(count);
    }
    
    void resize(size_t count, const T& value) {
        DEBUG_ALLOC(count * sizeof(T));
        base_type::resize(count, value);
    }
    
    void reserve(size_t new_cap) {
        DEBUG_ALLOC(new_cap * sizeof(T));
        base_type::reserve(new_cap);
    }
};

// String
template<typename CharT, typename Traits = std::char_traits<CharT>, typename Alloc = SimpleAllocator<CharT>>
class basic_string : public std::basic_string<CharT, Traits, Alloc> {
public:
    using base_type = std::basic_string<CharT, Traits, Alloc>;
    
    basic_string() : base_type() {}
    basic_string(size_t count, CharT ch) : base_type(count, ch) {
        DEBUG_ALLOC(count * sizeof(CharT));
    }
    basic_string(const basic_string& other) : base_type(other) {}
    basic_string(basic_string&& other) noexcept : base_type(std::move(other)) {}
    
    basic_string& operator=(const basic_string& other) {
        base_type::operator=(other);
        return *this;
    }
    
    basic_string& operator=(basic_string&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
    
    void resize(size_t count) {
        DEBUG_ALLOC(count * sizeof(CharT));
        base_type::resize(count);
    }
    
    void resize(size_t count, CharT ch) {
        DEBUG_ALLOC(count * sizeof(CharT));
        base_type::resize(count, ch);
    }
    
    void reserve(size_t new_cap) {
        DEBUG_ALLOC(new_cap * sizeof(CharT));
        base_type::reserve(new_cap);
    }
};

using string = basic_string<char>;
using wstring = basic_string<wchar_t>;

// Map
template<typename Key, typename T, typename Compare = std::less<Key>, typename Alloc = SimpleAllocator<std::pair<const Key, T>>>
class map : public std::map<Key, T, Compare, Alloc> {
public:
    using base_type = std::map<Key, T, Compare, Alloc>;
    
    map() : base_type() {}
    map(const map& other) : base_type(other) {}
    map(map&& other) noexcept : base_type(std::move(other)) {}
    
    map& operator=(const map& other) {
        base_type::operator=(other);
        return *this;
    }
    
    map& operator=(map&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
};

// Set
template<typename Key, typename Compare = std::less<Key>, typename Alloc = SimpleAllocator<Key>>
class set : public std::set<Key, Compare, Alloc> {
public:
    using base_type = std::set<Key, Compare, Alloc>;
    
    set() : base_type() {}
    set(const set& other) : base_type(other) {}
    set(set&& other) noexcept : base_type(std::move(other)) {}
    
    set& operator=(const set& other) {
        base_type::operator=(other);
        return *this;
    }
    
    set& operator=(set&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
};

// Multiset
template<typename Key, typename Compare = std::less<Key>, typename Alloc = SimpleAllocator<Key>>
class multiset : public std::multiset<Key, Compare, Alloc> {
public:
    using base_type = std::multiset<Key, Compare, Alloc>;
    
    multiset() : base_type() {}
    multiset(const multiset& other) : base_type(other) {}
    multiset(multiset&& other) noexcept : base_type(std::move(other)) {}
    
    multiset& operator=(const multiset& other) {
        base_type::operator=(other);
        return *this;
    }
    
    multiset& operator=(multiset&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
};

// Multimap
template<typename Key, typename T, typename Compare = std::less<Key>, typename Alloc = SimpleAllocator<std::pair<const Key, T>>>
class multimap : public std::multimap<Key, T, Compare, Alloc> {
public:
    using base_type = std::multimap<Key, T, Compare, Alloc>;
    
    multimap() : base_type() {}
    multimap(const multimap& other) : base_type(other) {}
    multimap(multimap&& other) noexcept : base_type(std::move(other)) {}
    
    multimap& operator=(const multimap& other) {
        base_type::operator=(other);
        return *this;
    }
    
    multimap& operator=(multimap&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
};

// Unordered Map
template<typename Key, typename T, typename Hash = std::hash<Key>, typename Pred = std::equal_to<Key>, typename Alloc = SimpleAllocator<std::pair<const Key, T>>>
class unordered_map : public std::unordered_map<Key, T, Hash, Pred, Alloc> {
public:
    using base_type = std::unordered_map<Key, T, Hash, Pred, Alloc>;
    
    unordered_map() : base_type() {}
    unordered_map(const unordered_map& other) : base_type(other) {}
    unordered_map(unordered_map&& other) noexcept : base_type(std::move(other)) {}
    
    unordered_map& operator=(const unordered_map& other) {
        base_type::operator=(other);
        return *this;
    }
    
    unordered_map& operator=(unordered_map&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
    
    void reserve(size_t count) {
        DEBUG_ALLOC(count * sizeof(std::pair<const Key, T>));
        base_type::reserve(count);
    }
};

// Unordered Set
template<typename Key, typename Hash = std::hash<Key>, typename Pred = std::equal_to<Key>, typename Alloc = SimpleAllocator<Key>>
class unordered_set : public std::unordered_set<Key, Hash, Pred, Alloc> {
public:
    using base_type = std::unordered_set<Key, Hash, Pred, Alloc>;
    
    unordered_set() : base_type() {}
    unordered_set(const unordered_set& other) : base_type(other) {}
    unordered_set(unordered_set&& other) noexcept : base_type(std::move(other)) {}
    
    unordered_set& operator=(const unordered_set& other) {
        base_type::operator=(other);
        return *this;
    }
    
    unordered_set& operator=(unordered_set&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
    
    void reserve(size_t count) {
        DEBUG_ALLOC(count * sizeof(Key));
        base_type::reserve(count);
    }
};

// Unordered Multiset
template<typename Key, typename Hash = std::hash<Key>, typename Pred = std::equal_to<Key>, typename Alloc = SimpleAllocator<Key>>
class unordered_multiset : public std::unordered_multiset<Key, Hash, Pred, Alloc> {
public:
    using base_type = std::unordered_multiset<Key, Hash, Pred, Alloc>;
    
    unordered_multiset() : base_type() {}
    unordered_multiset(const unordered_multiset& other) : base_type(other) {}
    unordered_multiset(unordered_multiset&& other) noexcept : base_type(std::move(other)) {}
    
    unordered_multiset& operator=(const unordered_multiset& other) {
        base_type::operator=(other);
        return *this;
    }
    
    unordered_multiset& operator=(unordered_multiset&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
    
    void reserve(size_t count) {
        DEBUG_ALLOC(count * sizeof(Key));
        base_type::reserve(count);
    }
};

// Unordered Multimap
template<typename Key, typename T, typename Hash = std::hash<Key>, typename Pred = std::equal_to<Key>, typename Alloc = SimpleAllocator<std::pair<const Key, T>>>
class unordered_multimap : public std::unordered_multimap<Key, T, Hash, Pred, Alloc> {
public:
    using base_type = std::unordered_multimap<Key, T, Hash, Pred, Alloc>;
    
    unordered_multimap() : base_type() {}
    unordered_multimap(const unordered_multimap& other) : base_type(other) {}
    unordered_multimap(unordered_multimap&& other) noexcept : base_type(std::move(other)) {}
    
    unordered_multimap& operator=(const unordered_multimap& other) {
        base_type::operator=(other);
        return *this;
    }
    
    unordered_multimap& operator=(unordered_multimap&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
    
    void reserve(size_t count) {
        DEBUG_ALLOC(count * sizeof(std::pair<const Key, T>));
        base_type::reserve(count);
    }
};

// List
template<typename T, typename Alloc = SimpleAllocator<T>>
class list : public std::list<T, Alloc> {
public:
    using base_type = std::list<T, Alloc>;
    
    list() : base_type() {}
    list(size_t count) : base_type(count) {
        DEBUG_ALLOC(count * sizeof(T));
    }
    list(size_t count, const T& value) : base_type(count, value) {
        DEBUG_ALLOC(count * sizeof(T));
    }
    list(const list& other) : base_type(other) {}
    list(list&& other) noexcept : base_type(std::move(other)) {}
    
    list& operator=(const list& other) {
        base_type::operator=(other);
        return *this;
    }
    
    list& operator=(list&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
};

// Deque
template<typename T, typename Alloc = SimpleAllocator<T>>
class deque : public std::deque<T, Alloc> {
public:
    using base_type = std::deque<T, Alloc>;
    
    deque() : base_type() {}
    deque(size_t count) : base_type(count) {
        DEBUG_ALLOC(count * sizeof(T));
    }
    deque(size_t count, const T& value) : base_type(count, value) {
        DEBUG_ALLOC(count * sizeof(T));
    }
    deque(const deque& other) : base_type(other) {}
    deque(deque&& other) noexcept : base_type(std::move(other)) {}
    
    deque& operator=(const deque& other) {
        base_type::operator=(other);
        return *this;
    }
    
    deque& operator=(deque&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
    
    void resize(size_t count) {
        DEBUG_ALLOC(count * sizeof(T));
        base_type::resize(count);
    }
    
    void resize(size_t count, const T& value) {
        DEBUG_ALLOC(count * sizeof(T));
        base_type::resize(count, value);
    }
};

// Queue (wrapper around deque)
template<typename T, typename Container = deque<T>>
class queue : public std::queue<T, Container> {
public:
    using base_type = std::queue<T, Container>;
    
    queue() : base_type() {}
    queue(const queue& other) : base_type(other) {}
    queue(queue&& other) noexcept : base_type(std::move(other)) {}
    
    queue& operator=(const queue& other) {
        base_type::operator=(other);
        return *this;
    }
    
    queue& operator=(queue&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
};

// Stack (wrapper around deque)
template<typename T, typename Container = deque<T>>
class stack : public std::stack<T, Container> {
public:
    using base_type = std::stack<T, Container>;
    
    stack() : base_type() {}
    stack(const stack& other) : base_type(other) {}
    stack(stack&& other) noexcept : base_type(std::move(other)) {}
    
    stack& operator=(const stack& other) {
        base_type::operator=(other);
        return *this;
    }
    
    stack& operator=(stack&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
};

// Priority Queue
template<typename T, typename Container = vector<T>, typename Compare = std::less<typename Container::value_type>>
class priority_queue : public std::priority_queue<T, Container, Compare> {
public:
    using base_type = std::priority_queue<T, Container, Compare>;
    
    priority_queue() : base_type() {}
    priority_queue(const priority_queue& other) : base_type(other) {}
    priority_queue(priority_queue&& other) noexcept : base_type(std::move(other)) {}
    
    priority_queue& operator=(const priority_queue& other) {
        base_type::operator=(other);
        return *this;
    }
    
    priority_queue& operator=(priority_queue&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
};

} // namespace Debug

#endif // SIMPLE_DEBUG_CONTAINERS_HPP
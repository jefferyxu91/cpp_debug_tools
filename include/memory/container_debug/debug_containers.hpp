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
#include <functional>

// Macro to capture caller's file, line, and function
#define DEBUG_CONTAINER_ALLOC(size) \
    Debug::print_allocation_info(size, __FILE__, __LINE__, __FUNCTION__)

// Macro for container instantiation that captures caller context
#define DEBUG_VECTOR(type, name) \
    Debug::vector<type> name(__FILE__, __LINE__, __FUNCTION__)

#define DEBUG_STRING(name) \
    Debug::string name(__FILE__, __LINE__, __FUNCTION__)

#define DEBUG_MAP(key_type, value_type, name) \
    Debug::map<key_type, value_type> name(__FILE__, __LINE__, __FUNCTION__)

#define DEBUG_LIST(type, name) \
    Debug::list<type> name(__FILE__, __LINE__, __FUNCTION__)

#define DEBUG_DEQUE(type, name) \
    Debug::deque<type> name(__FILE__, __LINE__, __FUNCTION__)

#define DEBUG_SET(type, name) \
    Debug::set<type> name(__FILE__, __LINE__, __FUNCTION__)

#define DEBUG_MULTISET(type, name) \
    Debug::multiset<type> name(__FILE__, __LINE__, __FUNCTION__)

#define DEBUG_MULTIMAP(key_type, value_type, name) \
    Debug::multimap<key_type, value_type> name(__FILE__, __LINE__, __FUNCTION__)

#define DEBUG_UNORDERED_SET(type, name) \
    Debug::unordered_set<type> name(__FILE__, __LINE__, __FUNCTION__)

#define DEBUG_UNORDERED_MULTISET(type, name) \
    Debug::unordered_multiset<type> name(__FILE__, __LINE__, __FUNCTION__)

#define DEBUG_UNORDERED_MAP(key_type, value_type, name) \
    Debug::unordered_map<key_type, value_type> name(__FILE__, __LINE__, __FUNCTION__)

#define DEBUG_UNORDERED_MULTIMAP(key_type, value_type, name) \
    Debug::unordered_multimap<key_type, value_type> name(__FILE__, __LINE__, __FUNCTION__)

namespace Debug {

// Configuration
constexpr size_t DEFAULT_MEMORY_THRESHOLD = 20 * 1024 * 1024; // 20MB

// Global threshold setting
inline size_t memory_threshold = DEFAULT_MEMORY_THRESHOLD;

// Global output stream function - defaults to std::cout
inline std::function<void(const std::string&)> output_stream = [](const std::string& message) {
    std::cout << message << std::endl;
};

// Utility function to print allocation info
inline void print_allocation_info(size_t size, const char* file, int line, const char* function) {
    if (size > memory_threshold) {
        std::string message = "[DEBUG] Large allocation detected: " + 
                             std::to_string(size) + " bytes at " + 
                             std::string(file) + ":" + std::to_string(line) + 
                             " in function '" + std::string(function) + "'";
        output_stream(message);
    }
}

// Function to set custom output stream
inline void set_output_stream(std::function<void(const std::string&)> stream) {
    output_stream = stream;
}

// Function to set output to std::cout (default)
inline void set_output_to_cout() {
    output_stream = [](const std::string& message) {
        std::cout << message << std::endl;
    };
}

// Function to set output to std::cerr
inline void set_output_to_cerr() {
    output_stream = [](const std::string& message) {
        std::cerr << message << std::endl;
    };
}

// Function to set output to a file stream
inline void set_output_to_file(std::ostream& file_stream) {
    output_stream = [&file_stream](const std::string& message) {
        file_stream << message << std::endl;
    };
}

// Function to set output to a custom stream (like ROS logging)
template<typename StreamType>
inline void set_output_to_stream(StreamType& stream) {
    output_stream = [&stream](const std::string& message) {
        stream << message;
    };
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
        print_allocation_info(total_size, __FILE__, __LINE__, __FUNCTION__);
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

// Container wrapper classes that track constructor allocations
template<typename T>
class vector : public std::vector<T, DebugAllocator<T>> {
public:
    using base_type = std::vector<T, DebugAllocator<T>>;
    
    // Default constructor
    vector() : base_type() {}
    
    // Constructor with caller context
    vector(const char* file, int line, const char* function) 
        : base_type(), caller_file_(file), caller_line_(line), caller_function_(function) {}
    
    // Constructor with size and caller context
    vector(size_t count, const char* file, int line, const char* function) 
        : base_type(count), caller_file_(file), caller_line_(line), caller_function_(function) {
        size_t total_size = count * sizeof(T);
        print_allocation_info(total_size, file, line, function);
    }
    
    // Constructor with size - tracks allocation
    explicit vector(size_t count) : base_type(count) {
        size_t total_size = count * sizeof(T);
        DEBUG_CONTAINER_ALLOC(total_size);
    }
    
    // Constructor with size and value - tracks allocation
    vector(size_t count, const T& value) : base_type(count, value) {
        size_t total_size = count * sizeof(T);
        if (caller_file_) {
            print_allocation_info(total_size, caller_file_, caller_line_, caller_function_);
        } else {
            DEBUG_CONTAINER_ALLOC(total_size);
        }
    }
    
    // Copy constructor - tracks allocation if large
    vector(const vector& other) : base_type(other), 
        caller_file_(other.caller_file_), caller_line_(other.caller_line_), caller_function_(other.caller_function_) {
        size_t total_size = other.size() * sizeof(T);
        if (caller_file_) {
            print_allocation_info(total_size, caller_file_, caller_line_, caller_function_);
        } else {
            DEBUG_CONTAINER_ALLOC(total_size);
        }
    }
    
    // Move constructor
    vector(vector&& other) noexcept : base_type(std::move(other)),
        caller_file_(other.caller_file_), caller_line_(other.caller_line_), caller_function_(other.caller_function_) {}
    
    // Assignment operators
    vector& operator=(const vector& other) {
        if (this != &other) {
            size_t total_size = other.size() * sizeof(T);
            if (caller_file_) {
                print_allocation_info(total_size, caller_file_, caller_line_, caller_function_);
            } else {
                DEBUG_CONTAINER_ALLOC(total_size);
            }
            caller_file_ = other.caller_file_;
            caller_line_ = other.caller_line_;
            caller_function_ = other.caller_function_;
            base_type::operator=(other);
        }
        return *this;
    }
    
    vector& operator=(vector&& other) noexcept {
        caller_file_ = other.caller_file_;
        caller_line_ = other.caller_line_;
        caller_function_ = other.caller_function_;
        base_type::operator=(std::move(other));
        return *this;
    }
    
    // Reserve method override
    void reserve(size_t new_cap) {
        size_t total_size = new_cap * sizeof(T);
        if (caller_file_) {
            print_allocation_info(total_size, caller_file_, caller_line_, caller_function_);
        } else {
            DEBUG_CONTAINER_ALLOC(total_size);
        }
        base_type::reserve(new_cap);
    }
    
    // Resize method override
    void resize(size_t count) {
        size_t total_size = count * sizeof(T);
        if (caller_file_) {
            print_allocation_info(total_size, caller_file_, caller_line_, caller_function_);
        } else {
            DEBUG_CONTAINER_ALLOC(total_size);
        }
        base_type::resize(count);
    }
    
    void resize(size_t count, const T& value) {
        size_t total_size = count * sizeof(T);
        if (caller_file_) {
            print_allocation_info(total_size, caller_file_, caller_line_, caller_function_);
        } else {
            DEBUG_CONTAINER_ALLOC(total_size);
        }
        base_type::resize(count, value);
    }

private:
    const char* caller_file_ = nullptr;
    int caller_line_ = 0;
    const char* caller_function_ = nullptr;
};

template<typename T>
class list : public std::list<T, DebugAllocator<T>> {
public:
    using base_type = std::list<T, DebugAllocator<T>>;
    using base_type::base_type;
    
    // Constructor with size - tracks allocation
    explicit list(size_t count) : base_type(count) {
        size_t total_size = count * sizeof(T);
        DEBUG_CONTAINER_ALLOC(total_size);
    }
    
    // Constructor with size and value - tracks allocation
    list(size_t count, const T& value) : base_type(count, value) {
        size_t total_size = count * sizeof(T);
        DEBUG_CONTAINER_ALLOC(total_size);
    }
    
    // Copy constructor - tracks allocation if large
    list(const list& other) : base_type(other) {
        size_t total_size = other.size() * sizeof(T);
        DEBUG_CONTAINER_ALLOC(total_size);
    }
    
    // Move constructor
    list(list&& other) noexcept : base_type(std::move(other)) {}
    
    // Assignment operators
    list& operator=(const list& other) {
        if (this != &other) {
            size_t total_size = other.size() * sizeof(T);
            DEBUG_CONTAINER_ALLOC(total_size);
            base_type::operator=(other);
        }
        return *this;
    }
    
    list& operator=(list&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
};

template<typename T>
class deque : public std::deque<T, DebugAllocator<T>> {
public:
    using base_type = std::deque<T, DebugAllocator<T>>;
    using base_type::base_type;
    
    // Constructor with size - tracks allocation
    explicit deque(size_t count) : base_type(count) {
        size_t total_size = count * sizeof(T);
        DEBUG_CONTAINER_ALLOC(total_size);
    }
    
    // Constructor with size and value - tracks allocation
    deque(size_t count, const T& value) : base_type(count, value) {
        size_t total_size = count * sizeof(T);
        DEBUG_CONTAINER_ALLOC(total_size);
    }
    
    // Copy constructor - tracks allocation if large
    deque(const deque& other) : base_type(other) {
        size_t total_size = other.size() * sizeof(T);
        DEBUG_CONTAINER_ALLOC(total_size);
    }
    
    // Move constructor
    deque(deque&& other) noexcept : base_type(std::move(other)) {}
    
    // Assignment operators
    deque& operator=(const deque& other) {
        if (this != &other) {
            size_t total_size = other.size() * sizeof(T);
            DEBUG_CONTAINER_ALLOC(total_size);
            base_type::operator=(other);
        }
        return *this;
    }
    
    deque& operator=(deque&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
};

template<typename T>
class set : public std::set<T, std::less<T>, DebugAllocator<T>> {
public:
    using base_type = std::set<T, std::less<T>, DebugAllocator<T>>;
    using base_type::base_type;
    
    // Copy constructor - tracks allocation if large
    set(const set& other) : base_type(other) {
        size_t total_size = other.size() * sizeof(T);
        DEBUG_CONTAINER_ALLOC(total_size);
    }
    
    // Move constructor
    set(set&& other) noexcept : base_type(std::move(other)) {}
    
    // Assignment operators
    set& operator=(const set& other) {
        if (this != &other) {
            size_t total_size = other.size() * sizeof(T);
            DEBUG_CONTAINER_ALLOC(total_size);
            base_type::operator=(other);
        }
        return *this;
    }
    
    set& operator=(set&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
};

template<typename T>
class multiset : public std::multiset<T, std::less<T>, DebugAllocator<T>> {
public:
    using base_type = std::multiset<T, std::less<T>, DebugAllocator<T>>;
    using base_type::base_type;
    
    // Copy constructor - tracks allocation if large
    multiset(const multiset& other) : base_type(other) {
        size_t total_size = other.size() * sizeof(T);
        DEBUG_CONTAINER_ALLOC(total_size);
    }
    
    // Move constructor
    multiset(multiset&& other) noexcept : base_type(std::move(other)) {}
    
    // Assignment operators
    multiset& operator=(const multiset& other) {
        if (this != &other) {
            size_t total_size = other.size() * sizeof(T);
            DEBUG_CONTAINER_ALLOC(total_size);
            base_type::operator=(other);
        }
        return *this;
    }
    
    multiset& operator=(multiset&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
};

template<typename Key, typename Value>
class map : public std::map<Key, Value, std::less<Key>, DebugAllocator<std::pair<const Key, Value>>> {
public:
    using base_type = std::map<Key, Value, std::less<Key>, DebugAllocator<std::pair<const Key, Value>>>;
    
    // Default constructor
    map() : base_type() {}
    
    // Constructor with caller context
    map(const char* file, int line, const char* function) 
        : base_type(), caller_file_(file), caller_line_(line), caller_function_(function) {}
    
    // Copy constructor - tracks allocation if large
    map(const map& other) : base_type(other), 
        caller_file_(other.caller_file_), caller_line_(other.caller_line_), caller_function_(other.caller_function_) {
        size_t total_size = other.size() * sizeof(std::pair<const Key, Value>);
        if (caller_file_) {
            print_allocation_info(total_size, caller_file_, caller_line_, caller_function_);
        } else {
            DEBUG_CONTAINER_ALLOC(total_size);
        }
    }
    
    // Move constructor
    map(map&& other) noexcept : base_type(std::move(other)),
        caller_file_(other.caller_file_), caller_line_(other.caller_line_), caller_function_(other.caller_function_) {}
    
    // Assignment operators
    map& operator=(const map& other) {
        if (this != &other) {
            size_t total_size = other.size() * sizeof(std::pair<const Key, Value>);
            if (caller_file_) {
                print_allocation_info(total_size, caller_file_, caller_line_, caller_function_);
            } else {
                DEBUG_CONTAINER_ALLOC(total_size);
            }
            caller_file_ = other.caller_file_;
            caller_line_ = other.caller_line_;
            caller_function_ = other.caller_function_;
            base_type::operator=(other);
        }
        return *this;
    }
    
    map& operator=(map&& other) noexcept {
        caller_file_ = other.caller_file_;
        caller_line_ = other.caller_line_;
        caller_function_ = other.caller_function_;
        base_type::operator=(std::move(other));
        return *this;
    }

private:
    const char* caller_file_ = nullptr;
    int caller_line_ = 0;
    const char* caller_function_ = nullptr;
};

template<typename Key, typename Value>
class multimap : public std::multimap<Key, Value, std::less<Key>, DebugAllocator<std::pair<const Key, Value>>> {
public:
    using base_type = std::multimap<Key, Value, std::less<Key>, DebugAllocator<std::pair<const Key, Value>>>;
    using base_type::base_type;
    
    // Copy constructor - tracks allocation if large
    multimap(const multimap& other) : base_type(other) {
        size_t total_size = other.size() * sizeof(std::pair<const Key, Value>);
        DEBUG_CONTAINER_ALLOC(total_size);
    }
    
    // Move constructor
    multimap(multimap&& other) noexcept : base_type(std::move(other)) {}
    
    // Assignment operators
    multimap& operator=(const multimap& other) {
        if (this != &other) {
            size_t total_size = other.size() * sizeof(std::pair<const Key, Value>);
            DEBUG_CONTAINER_ALLOC(total_size);
            base_type::operator=(other);
        }
        return *this;
    }
    
    multimap& operator=(multimap&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
};

template<typename T>
class unordered_set : public std::unordered_set<T, std::hash<T>, std::equal_to<T>, DebugAllocator<T>> {
public:
    using base_type = std::unordered_set<T, std::hash<T>, std::equal_to<T>, DebugAllocator<T>>;
    using base_type::base_type;
    
    // Copy constructor - tracks allocation if large
    unordered_set(const unordered_set& other) : base_type(other) {
        size_t total_size = other.size() * sizeof(T);
        DEBUG_CONTAINER_ALLOC(total_size);
    }
    
    // Move constructor
    unordered_set(unordered_set&& other) noexcept : base_type(std::move(other)) {}
    
    // Assignment operators
    unordered_set& operator=(const unordered_set& other) {
        if (this != &other) {
            size_t total_size = other.size() * sizeof(T);
            DEBUG_CONTAINER_ALLOC(total_size);
            base_type::operator=(other);
        }
        return *this;
    }
    
    unordered_set& operator=(unordered_set&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
    
    // Reserve method override
    void reserve(size_t count) {
        size_t total_size = count * sizeof(T);
        DEBUG_CONTAINER_ALLOC(total_size);
        base_type::reserve(count);
    }
};

template<typename T>
class unordered_multiset : public std::unordered_multiset<T, std::hash<T>, std::equal_to<T>, DebugAllocator<T>> {
public:
    using base_type = std::unordered_multiset<T, std::hash<T>, std::equal_to<T>, DebugAllocator<T>>;
    using base_type::base_type;
    
    // Copy constructor - tracks allocation if large
    unordered_multiset(const unordered_multiset& other) : base_type(other) {
        size_t total_size = other.size() * sizeof(T);
        DEBUG_CONTAINER_ALLOC(total_size);
    }
    
    // Move constructor
    unordered_multiset(unordered_multiset&& other) noexcept : base_type(std::move(other)) {}
    
    // Assignment operators
    unordered_multiset& operator=(const unordered_multiset& other) {
        if (this != &other) {
            size_t total_size = other.size() * sizeof(T);
            DEBUG_CONTAINER_ALLOC(total_size);
            base_type::operator=(other);
        }
        return *this;
    }
    
    unordered_multiset& operator=(unordered_multiset&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
    
    // Reserve method override
    void reserve(size_t count) {
        size_t total_size = count * sizeof(T);
        DEBUG_CONTAINER_ALLOC(total_size);
        base_type::reserve(count);
    }
};

template<typename Key, typename Value>
class unordered_map : public std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>, 
                                               DebugAllocator<std::pair<const Key, Value>>> {
public:
    using base_type = std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>, 
                                        DebugAllocator<std::pair<const Key, Value>>>;
    using base_type::base_type;
    
    // Copy constructor - tracks allocation if large
    unordered_map(const unordered_map& other) : base_type(other) {
        size_t total_size = other.size() * sizeof(std::pair<const Key, Value>);
        DEBUG_CONTAINER_ALLOC(total_size);
    }
    
    // Move constructor
    unordered_map(unordered_map&& other) noexcept : base_type(std::move(other)) {}
    
    // Assignment operators
    unordered_map& operator=(const unordered_map& other) {
        if (this != &other) {
            size_t total_size = other.size() * sizeof(std::pair<const Key, Value>);
            DEBUG_CONTAINER_ALLOC(total_size);
            base_type::operator=(other);
        }
        return *this;
    }
    
    unordered_map& operator=(unordered_map&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
    
    // Reserve method override
    void reserve(size_t count) {
        size_t total_size = count * sizeof(std::pair<const Key, Value>);
        DEBUG_CONTAINER_ALLOC(total_size);
        base_type::reserve(count);
    }
};

template<typename Key, typename Value>
class unordered_multimap : public std::unordered_multimap<Key, Value, std::hash<Key>, std::equal_to<Key>, 
                                                        DebugAllocator<std::pair<const Key, Value>>> {
public:
    using base_type = std::unordered_multimap<Key, Value, std::hash<Key>, std::equal_to<Key>, 
                                             DebugAllocator<std::pair<const Key, Value>>>;
    using base_type::base_type;
    
    // Copy constructor - tracks allocation if large
    unordered_multimap(const unordered_multimap& other) : base_type(other) {
        size_t total_size = other.size() * sizeof(std::pair<const Key, Value>);
        DEBUG_CONTAINER_ALLOC(total_size);
    }
    
    // Move constructor
    unordered_multimap(unordered_multimap&& other) noexcept : base_type(std::move(other)) {}
    
    // Assignment operators
    unordered_multimap& operator=(const unordered_multimap& other) {
        if (this != &other) {
            size_t total_size = other.size() * sizeof(std::pair<const Key, Value>);
            DEBUG_CONTAINER_ALLOC(total_size);
            base_type::operator=(other);
        }
        return *this;
    }
    
    unordered_multimap& operator=(unordered_multimap&& other) noexcept {
        base_type::operator=(std::move(other));
        return *this;
    }
    
    // Reserve method override
    void reserve(size_t count) {
        size_t total_size = count * sizeof(std::pair<const Key, Value>);
        DEBUG_CONTAINER_ALLOC(total_size);
        base_type::reserve(count);
    }
};

// Container adaptors
template<typename T>
using stack = std::stack<T, deque<T>>;

template<typename T>
using queue = std::queue<T, deque<T>>;

template<typename T>
using priority_queue = std::priority_queue<T, vector<T>>;

// String wrapper
class string : public std::basic_string<char, std::char_traits<char>, DebugAllocator<char>> {
public:
    using base_type = std::basic_string<char, std::char_traits<char>, DebugAllocator<char>>;
    
    // Default constructor
    string() : base_type() {}
    
    // Constructor with caller context
    string(const char* file, int line, const char* function) 
        : base_type(), caller_file_(file), caller_line_(line), caller_function_(function) {}
    
    // Constructor with count and caller context
    string(size_t count, char ch, const char* file, int line, const char* function) 
        : base_type(count, ch), caller_file_(file), caller_line_(line), caller_function_(function) {
        size_t total_size = count * sizeof(char);
        print_allocation_info(total_size, file, line, function);
    }
    
    // Constructor with count - tracks allocation
    explicit string(size_t count, char ch = char()) : base_type(count, ch) {
        size_t total_size = count * sizeof(char);
        DEBUG_CONTAINER_ALLOC(total_size);
    }
    
    // Copy constructor - tracks allocation if large
    string(const string& other) : base_type(other), 
        caller_file_(other.caller_file_), caller_line_(other.caller_line_), caller_function_(other.caller_function_) {
        size_t total_size = other.size() * sizeof(char);
        if (caller_file_) {
            print_allocation_info(total_size, caller_file_, caller_line_, caller_function_);
        } else {
            DEBUG_CONTAINER_ALLOC(total_size);
        }
    }
    
    // Move constructor
    string(string&& other) noexcept : base_type(std::move(other)),
        caller_file_(other.caller_file_), caller_line_(other.caller_line_), caller_function_(other.caller_function_) {}
    
    // Assignment operators
    string& operator=(const string& other) {
        if (this != &other) {
            size_t total_size = other.size() * sizeof(char);
            if (caller_file_) {
                print_allocation_info(total_size, caller_file_, caller_line_, caller_function_);
            } else {
                DEBUG_CONTAINER_ALLOC(total_size);
            }
            caller_file_ = other.caller_file_;
            caller_line_ = other.caller_line_;
            caller_function_ = other.caller_function_;
            base_type::operator=(other);
        }
        return *this;
    }
    
    string& operator=(string&& other) noexcept {
        caller_file_ = other.caller_file_;
        caller_line_ = other.caller_line_;
        caller_function_ = other.caller_function_;
        base_type::operator=(std::move(other));
        return *this;
    }
    
    // Reserve method override
    void reserve(size_t new_cap) {
        size_t total_size = new_cap * sizeof(char);
        if (caller_file_) {
            print_allocation_info(total_size, caller_file_, caller_line_, caller_function_);
        } else {
            DEBUG_CONTAINER_ALLOC(total_size);
        }
        base_type::reserve(new_cap);
    }

private:
    const char* caller_file_ = nullptr;
    int caller_line_ = 0;
    const char* caller_function_ = nullptr;
};

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
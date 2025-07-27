#include "debug_containers.hpp"
#include <iostream>

int main() {
    // Set a lower threshold for demonstration
    Debug::set_memory_threshold(512 * 1024); // 512KB
    
    std::cout << "=== Debug Containers Test ===" << std::endl;
    std::cout << "Memory threshold: " << Debug::get_memory_threshold() << " bytes" << std::endl << std::endl;
    
    // Test vector
    std::cout << "Testing Debug::vector..." << std::endl;
    Debug::vector<int> vec;
    vec.reserve(100000); // Should trigger debug output
    
    // Test list
    std::cout << "Testing Debug::list..." << std::endl;
    Debug::list<int> lst;
    for (int i = 0; i < 50000; ++i) {
        lst.push_back(i);
    }
    
    // Test map
    std::cout << "Testing Debug::map..." << std::endl;
    Debug::map<int, std::string> map_test;
    for (int i = 0; i < 10000; ++i) {
        map_test[i] = "value" + std::to_string(i);
    }
    
    // Test unordered_map
    std::cout << "Testing Debug::unordered_map..." << std::endl;
    Debug::unordered_map<int, std::string> umap_test;
    umap_test.reserve(50000); // Should trigger debug output
    
    // Test set
    std::cout << "Testing Debug::set..." << std::endl;
    Debug::set<int> set_test;
    for (int i = 0; i < 20000; ++i) {
        set_test.insert(i);
    }
    
    // Test string
    std::cout << "Testing Debug::string..." << std::endl;
    Debug::string str_test;
    str_test.reserve(100000); // Should trigger debug output
    
    // Test stack
    std::cout << "Testing Debug::stack..." << std::endl;
    Debug::stack<int> stack_test;
    for (int i = 0; i < 10000; ++i) {
        stack_test.push(i);
    }
    
    // Test queue
    std::cout << "Testing Debug::queue..." << std::endl;
    Debug::queue<int> queue_test;
    for (int i = 0; i < 10000; ++i) {
        queue_test.push(i);
    }
    
    // Test priority_queue
    std::cout << "Testing Debug::priority_queue..." << std::endl;
    Debug::priority_queue<int> pq_test;
    for (int i = 0; i < 10000; ++i) {
        pq_test.push(i);
    }
    
    std::cout << std::endl << "All tests completed successfully!" << std::endl;
    
    return 0;
}
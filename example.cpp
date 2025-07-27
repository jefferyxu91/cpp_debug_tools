#include "debug_containers.hpp"
#include <iostream>

int main() {
    // Set a lower threshold for demonstration (1MB instead of 20MB)
    Debug::set_memory_threshold(1024 * 1024); // 1MB
    
    std::cout << "Memory threshold set to: " << Debug::get_memory_threshold() << " bytes" << std::endl;
    
    // This will trigger the debug output (allocating ~2MB)
    Debug::vector<int> large_vector;
    large_vector.reserve(500000); // This should trigger the debug message
    
    // This won't trigger the debug output (small allocation)
    Debug::vector<int> small_vector;
    small_vector.reserve(1000);
    
    // Test with other containers
    Debug::map<int, std::string> debug_map;
    Debug::unordered_set<int> debug_set;
    Debug::string debug_string;
    
    std::cout << "Example completed successfully!" << std::endl;
    
    return 0;
}
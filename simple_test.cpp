#include "debug_containers.hpp"
#include <iostream>

int main() {
    // Set a very low threshold for testing
    Debug::set_memory_threshold(1000); // 1KB
    
    std::cout << "Testing with threshold: " << Debug::get_memory_threshold() << " bytes" << std::endl;
    
    // This should definitely trigger the debug output
    Debug::vector<int> test_vec;
    std::cout << "About to reserve 10000 ints..." << std::endl;
    test_vec.reserve(10000); // 10000 * 4 bytes = 40KB, well above 1KB threshold
    
    std::cout << "Test completed!" << std::endl;
    
    return 0;
}
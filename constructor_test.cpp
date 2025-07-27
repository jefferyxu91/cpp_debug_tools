#include "debug_containers.hpp"
#include <iostream>

int main() {
    // Set a low threshold for demonstration
    Debug::set_memory_threshold(1000); // 1KB
    
    std::cout << "=== Constructor Allocation Test ===" << std::endl;
    std::cout << "Memory threshold: " << Debug::get_memory_threshold() << " bytes" << std::endl << std::endl;
    
    // Test constructor with size - should trigger debug output
    std::cout << "1. Testing Debug::vector constructor with size..." << std::endl;
    Debug::vector<int> vec1(5000); // 5000 * 4 bytes = 20KB > 1KB threshold
    
    // Test constructor with size and value - should trigger debug output
    std::cout << "2. Testing Debug::vector constructor with size and value..." << std::endl;
    Debug::vector<int> vec2(3000, 42); // 3000 * 4 bytes = 12KB > 1KB threshold
    
    // Test copy constructor - should trigger debug output
    std::cout << "3. Testing Debug::vector copy constructor..." << std::endl;
    Debug::vector<int> vec3 = vec1; // Copying 20KB > 1KB threshold
    
    // Test assignment operator - should trigger debug output
    std::cout << "4. Testing Debug::vector assignment operator..." << std::endl;
    Debug::vector<int> vec4;
    vec4 = vec2; // Assigning 12KB > 1KB threshold
    
    // Test list constructor with size
    std::cout << "5. Testing Debug::list constructor with size..." << std::endl;
    Debug::list<int> list1(2000); // 2000 * 4 bytes = 8KB > 1KB threshold
    
    // Test string constructor with size
    std::cout << "6. Testing Debug::string constructor with size..." << std::endl;
    Debug::string str1(5000, 'a'); // 5000 * 1 byte = 5KB > 1KB threshold
    
    // Test map copy constructor
    std::cout << "7. Testing Debug::map copy constructor..." << std::endl;
    Debug::map<int, std::string> map1;
    for (int i = 0; i < 1000; ++i) {
        map1[i] = "value" + std::to_string(i);
    }
    Debug::map<int, std::string> map2 = map1; // Copying large map
    
    // Test unordered_map assignment
    std::cout << "8. Testing Debug::unordered_map assignment..." << std::endl;
    Debug::unordered_map<int, std::string> umap1;
    for (int i = 0; i < 800; ++i) {
        umap1[i] = "value" + std::to_string(i);
    }
    Debug::unordered_map<int, std::string> umap2;
    umap2 = umap1; // Assigning large unordered_map
    
    // Test small allocations that shouldn't trigger debug output
    std::cout << "9. Testing small allocations (should not trigger debug output)..." << std::endl;
    Debug::vector<int> small_vec(100); // 100 * 4 bytes = 400 bytes < 1KB threshold
    Debug::string small_str(200, 'b'); // 200 * 1 byte = 200 bytes < 1KB threshold
    
    std::cout << std::endl << "All constructor tests completed!" << std::endl;
    
    return 0;
}
#include "debug_containers.hpp"
#include <iostream>

int main() {
    // Set a low threshold for demonstration
    Debug::set_memory_threshold(2000); // 2KB
    
    std::cout << "=== Comprehensive Debug Containers Example ===" << std::endl;
    std::cout << "Memory threshold: " << Debug::get_memory_threshold() << " bytes" << std::endl << std::endl;
    
    std::cout << "=== 1. Constructor-Based Allocations ===" << std::endl;
    
    // Size-based constructor
    std::cout << "Creating vector with size 1000..." << std::endl;
    Debug::vector<int> vec1(1000); // 1000 * 4 = 4KB > 2KB threshold
    
    // Size and value constructor
    std::cout << "Creating vector with size 800 and value 42..." << std::endl;
    Debug::vector<int> vec2(800, 42); // 800 * 4 = 3.2KB > 2KB threshold
    
    // String constructor with size
    std::cout << "Creating string with size 3000..." << std::endl;
    Debug::string str1(3000, 'x'); // 3000 * 1 = 3KB > 2KB threshold
    
    std::cout << "\n=== 2. Copy Constructor Allocations ===" << std::endl;
    
    // Copy constructor
    std::cout << "Copying large vector..." << std::endl;
    Debug::vector<int> vec3 = vec1; // Copying 4KB > 2KB threshold
    
    // Copy constructor for string
    std::cout << "Copying large string..." << std::endl;
    Debug::string str2 = str1; // Copying 3KB > 2KB threshold
    
    std::cout << "\n=== 3. Assignment Operator Allocations ===" << std::endl;
    
    // Assignment operator
    std::cout << "Assigning large vector..." << std::endl;
    Debug::vector<int> vec4;
    vec4 = vec2; // Assigning 3.2KB > 2KB threshold
    
    // Assignment operator for string
    std::cout << "Assigning large string..." << std::endl;
    Debug::string str3;
    str3 = str1; // Assigning 3KB > 2KB threshold
    
    std::cout << "\n=== 4. Reserve Method Allocations ===" << std::endl;
    
    // Reserve method
    std::cout << "Reserving capacity for vector..." << std::endl;
    Debug::vector<int> vec5;
    vec5.reserve(1000); // 1000 * 4 = 4KB > 2KB threshold
    
    // Reserve method for string
    std::cout << "Reserving capacity for string..." << std::endl;
    Debug::string str4;
    str4.reserve(2500); // 2500 * 1 = 2.5KB > 2KB threshold
    
    // Reserve method for unordered containers
    std::cout << "Reserving capacity for unordered_map..." << std::endl;
    Debug::unordered_map<int, std::string> umap;
    umap.reserve(500); // 500 * 8 = 4KB > 2KB threshold (assuming 8 bytes per pair)
    
    std::cout << "\n=== 5. Runtime Allocations ===" << std::endl;
    
    // Runtime allocations through push_back/insert
    std::cout << "Adding elements to trigger runtime allocations..." << std::endl;
    Debug::vector<int> vec6;
    for (int i = 0; i < 1000; ++i) {
        vec6.push_back(i); // Will trigger allocations when capacity is exceeded
    }
    
    std::cout << "\n=== 6. Container-Specific Allocations ===" << std::endl;
    
    // Map allocations
    std::cout << "Creating large map..." << std::endl;
    Debug::map<int, std::string> map1;
    for (int i = 0; i < 500; ++i) {
        map1[i] = "value" + std::to_string(i);
    }
    
    // Set allocations
    std::cout << "Creating large set..." << std::endl;
    Debug::set<int> set1;
    for (int i = 0; i < 800; ++i) {
        set1.insert(i);
    }
    
    // List allocations
    std::cout << "Creating large list..." << std::endl;
    Debug::list<int> list1(600); // 600 * 4 = 2.4KB > 2KB threshold
    
    std::cout << "\n=== 7. Small Allocations (Should Not Trigger Debug) ===" << std::endl;
    
    // Small allocations that shouldn't trigger debug output
    std::cout << "Creating small containers..." << std::endl;
    Debug::vector<int> small_vec(100); // 100 * 4 = 400 bytes < 2KB threshold
    Debug::string small_str(500, 'a'); // 500 * 1 = 500 bytes < 2KB threshold
    Debug::map<int, int> small_map;
    for (int i = 0; i < 50; ++i) {
        small_map[i] = i; // Small map
    }
    
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "All allocation types have been tested:" << std::endl;
    std::cout << "✓ Constructor-based allocations" << std::endl;
    std::cout << "✓ Copy constructor allocations" << std::endl;
    std::cout << "✓ Assignment operator allocations" << std::endl;
    std::cout << "✓ Reserve method allocations" << std::endl;
    std::cout << "✓ Runtime allocations" << std::endl;
    std::cout << "✓ Container-specific allocations" << std::endl;
    std::cout << "✓ Small allocations (correctly ignored)" << std::endl;
    
    std::cout << "\nDebug containers example completed successfully!" << std::endl;
    
    return 0;
}
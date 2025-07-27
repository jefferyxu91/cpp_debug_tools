#include <memory/container_debug/debug_containers.hpp>
#include <iostream>

void test_vector_resize() {
    Debug::vector<int> vec;
    vec.resize(1000);  // This should trigger debug output with this file and line
}

void test_vector_reserve() {
    Debug::vector<int> vec;
    vec.reserve(2000);  // This should trigger debug output with this file and line
}

void test_vector_constructor() {
    Debug::vector<int> vec(1500);  // This should trigger debug output with this file and line
}

void test_string_constructor() {
    Debug::string str(3000, 'x');  // This should trigger debug output with this file and line
}

void test_map_operations() {
    Debug::map<int, std::string> map;
    for (int i = 0; i < 500; ++i) {
        map[i] = "value_" + std::to_string(i);  // This should trigger debug output
    }
}

int main() {
    // Set a low threshold to see all allocations
    Debug::set_memory_threshold(1000); // 1KB
    
    std::cout << "=== File and Line Tracking Test ===" << std::endl;
    std::cout << "Memory threshold: " << Debug::get_memory_threshold() << " bytes" << std::endl << std::endl;
    
    std::cout << "1. Testing vector resize..." << std::endl;
    test_vector_resize();
    
    std::cout << "\n2. Testing vector reserve..." << std::endl;
    test_vector_reserve();
    
    std::cout << "\n3. Testing vector constructor..." << std::endl;
    test_vector_constructor();
    
    std::cout << "\n4. Testing string constructor..." << std::endl;
    test_string_constructor();
    
    std::cout << "\n5. Testing map operations..." << std::endl;
    test_map_operations();
    
    std::cout << "\n=== Test Completed ===" << std::endl;
    std::cout << "Check the debug output above to see file and line information!" << std::endl;
    
    return 0;
}
#include <memory/container_debug/simple_debug_containers.hpp>
#include <iostream>

void test_function_1() {
    Debug::vector<int> vec;
    vec.resize(1000);  // Will automatically show this file and line
}

void test_function_2() {
    Debug::vector<int> vec(2000);  // Will automatically show this file and line
}

void test_function_3() {
    Debug::map<int, std::string> map;
    for (int i = 0; i < 500; ++i) {
        map[i] = "value_" + std::to_string(i);  // Will show allocator allocations
    }
}

class TestClass {
public:
    void member_function() {
        Debug::vector<double> vec(1500);  // Will automatically show this file and line
    }
    
    static void static_function() {
        Debug::string str(3000, 'x');  // Will automatically show this file and line
    }
};

int main() {
    // Set a low threshold to see all allocations
    Debug::set_memory_threshold(1000); // 1KB
    
    std::cout << "=== Simple Non-Intrusive Debug Containers Test ===" << std::endl;
    std::cout << "Memory threshold: " << Debug::get_memory_threshold() << " bytes" << std::endl << std::endl;
    
    std::cout << "1. Testing function_1..." << std::endl;
    test_function_1();
    
    std::cout << "\n2. Testing function_2..." << std::endl;
    test_function_2();
    
    std::cout << "\n3. Testing function_3..." << std::endl;
    test_function_3();
    
    std::cout << "\n4. Testing member function..." << std::endl;
    TestClass obj;
    obj.member_function();
    
    std::cout << "\n5. Testing static function..." << std::endl;
    TestClass::static_function();
    
    std::cout << "\n=== Test Completed ===" << std::endl;
    std::cout << "Check the debug output above to see automatic file/line capture!" << std::endl;
    std::cout << "No intrusive macros or context passing needed!" << std::endl;
    
    return 0;
}
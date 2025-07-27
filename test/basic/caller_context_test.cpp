#include <memory/container_debug/debug_containers.hpp>
#include <iostream>

void test_function_1() {
    Debug::vector<int> vec(__FILE__, __LINE__, __FUNCTION__);  // This should show function name 'test_function_1'
    vec.resize(1000);
}

void test_function_2() {
    Debug::vector<int> vec(2000, __FILE__, __LINE__, __FUNCTION__);  // This should show function name 'test_function_2'
}

void test_function_3() {
    Debug::map<int, std::string> map(__FILE__, __LINE__, __FUNCTION__);  // This should show function name 'test_function_3'
    for (int i = 0; i < 500; ++i) {
        map[i] = "value_" + std::to_string(i);
    }
}

class TestClass {
public:
    void member_function() {
        Debug::vector<double> vec(1500, __FILE__, __LINE__, __FUNCTION__);  // This should show function name 'TestClass::member_function'
    }
    
    static void static_function() {
        Debug::string str(3000, 'x', __FILE__, __LINE__, __FUNCTION__);  // This should show function name 'TestClass::static_function'
    }
};

int main() {
    // Set a low threshold to see all allocations
    Debug::set_memory_threshold(1000); // 1KB
    
    std::cout << "=== Caller Context Tracking Test ===" << std::endl;
    std::cout << "Memory threshold: " << Debug::get_memory_threshold() << " bytes" << std::endl << std::endl;
    
    std::cout << "1. Testing function_1 with DEBUG_VECTOR..." << std::endl;
    test_function_1();
    
    std::cout << "\n2. Testing function_2 with constructor context..." << std::endl;
    test_function_2();
    
    std::cout << "\n3. Testing function_3 with DEBUG_MAP..." << std::endl;
    test_function_3();
    
    std::cout << "\n4. Testing member function..." << std::endl;
    TestClass obj;
    obj.member_function();
    
    std::cout << "\n5. Testing static function..." << std::endl;
    TestClass::static_function();
    
    std::cout << "\n=== Test Completed ===" << std::endl;
    std::cout << "Check the debug output above to see caller context!" << std::endl;
    
    return 0;
}
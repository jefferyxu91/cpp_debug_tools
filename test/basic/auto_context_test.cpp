#include <memory/container_debug/debug_containers.hpp>
#include <iostream>

void test_function_1() {
    AUTO_CONTEXT();  // Automatically captures context
    Debug::vector<int> vec;  // Will use the captured context
    vec.resize(1000);
}

void test_function_2() {
    AUTO_CONTEXT();  // Automatically captures context
    Debug::vector<int> vec(2000);  // Will use the captured context
}

void test_function_3() {
    AUTO_CONTEXT();  // Automatically captures context
    Debug::map<int, std::string> map;  // Will use the captured context
    for (int i = 0; i < 500; ++i) {
        map[i] = "value_" + std::to_string(i);
    }
}

class TestClass {
public:
    void member_function() {
        AUTO_CONTEXT();  // Automatically captures context
        Debug::vector<double> vec(1500);  // Will use the captured context
    }
    
    static void static_function() {
        AUTO_CONTEXT();  // Automatically captures context
        Debug::string str(3000, 'x');  // Will use the captured context
    }
};

// Test with macro-based approach
void test_macro_approach() {
    DEBUG_VECTOR(int, vec1);  // Uses macro to capture context
    vec1.resize(1000);
    
    DEBUG_MAP(int, std::string, map1);  // Uses macro to capture context
    map1[1] = "test";
}

// Note: Auto-capture with std:: names is not recommended due to conflicts
// Use AUTO_CONTEXT() or DEBUG_* macros instead

int main() {
    // Set a low threshold to see all allocations
    Debug::set_memory_threshold(1000); // 1KB
    
    std::cout << "=== Automatic Context Capture Test ===" << std::endl;
    std::cout << "Memory threshold: " << Debug::get_memory_threshold() << " bytes" << std::endl << std::endl;
    
    std::cout << "1. Testing AUTO_CONTEXT() with function_1..." << std::endl;
    test_function_1();
    
    std::cout << "\n2. Testing AUTO_CONTEXT() with function_2..." << std::endl;
    test_function_2();
    
    std::cout << "\n3. Testing AUTO_CONTEXT() with function_3..." << std::endl;
    test_function_3();
    
    std::cout << "\n4. Testing AUTO_CONTEXT() with member function..." << std::endl;
    TestClass obj;
    obj.member_function();
    
    std::cout << "\n5. Testing AUTO_CONTEXT() with static function..." << std::endl;
    TestClass::static_function();
    
    std::cout << "\n6. Testing macro-based approach..." << std::endl;
    test_macro_approach();
    
    std::cout << "\n7. Auto-capture summary..." << std::endl;
    std::cout << "   - Use AUTO_CONTEXT() for automatic context capture" << std::endl;
    std::cout << "   - Use DEBUG_* macros for explicit context capture" << std::endl;
    std::cout << "   - Both approaches show caller's file, line, and function!" << std::endl;
    
    std::cout << "\n=== Test Completed ===" << std::endl;
    std::cout << "Check the debug output above to see automatic context capture!" << std::endl;
    
    return 0;
}
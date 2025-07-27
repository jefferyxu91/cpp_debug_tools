#include <gtest/gtest.h>
#include <memory/container_debug/debug_containers.hpp>
#include <iostream>
#include <sstream>

class DebugContainersTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set a low threshold for testing
        Debug::set_memory_threshold(1000); // 1KB
    }
    
    void TearDown() override {
        // Reset to default threshold
        Debug::set_memory_threshold(Debug::DEFAULT_MEMORY_THRESHOLD);
    }
};

// Test basic vector operations
TEST_F(DebugContainersTest, VectorBasicOperations) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test default constructor
    Debug::vector<int> vec1;
    EXPECT_TRUE(vec1.empty());
    
    // Test constructor with size - should trigger debug output
    Debug::vector<int> vec2(5000); // 5000 * 4 bytes = 20KB > 1KB threshold
    EXPECT_EQ(vec2.size(), 5000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    EXPECT_TRUE(output.str().find("vector") != std::string::npos);
    
    // Test constructor with size and value
    output.str(""); // Clear output
    Debug::vector<int> vec3(3000, 42); // 3000 * 4 bytes = 12KB > 1KB threshold
    EXPECT_EQ(vec3.size(), 3000);
    EXPECT_EQ(vec3[0], 42);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test copy constructor
    output.str(""); // Clear output
    Debug::vector<int> vec4 = vec2; // Copying 20KB > 1KB threshold
    EXPECT_EQ(vec4.size(), 5000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test assignment operator
    output.str(""); // Clear output
    Debug::vector<int> vec5;
    vec5 = vec3; // Assigning 12KB > 1KB threshold
    EXPECT_EQ(vec5.size(), 3000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test resize
    output.str(""); // Clear output
    vec1.resize(2000); // 2000 * 4 bytes = 8KB > 1KB threshold
    EXPECT_EQ(vec1.size(), 2000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    EXPECT_TRUE(output.str().find("resize") != std::string::npos);
    
    // Test reserve
    output.str(""); // Clear output
    vec1.reserve(3000); // 3000 * 4 bytes = 12KB > 1KB threshold
    EXPECT_GE(vec1.capacity(), 3000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    EXPECT_TRUE(output.str().find("reserve") != std::string::npos);
    
    // Test small allocation (should not trigger debug output)
    output.str(""); // Clear output
    Debug::vector<int> small_vec(100); // 100 * 4 bytes = 400 bytes < 1KB threshold
    EXPECT_EQ(small_vec.size(), 100);
    EXPECT_TRUE(output.str().empty()); // No debug output for small allocation
}

// Test string operations
TEST_F(DebugContainersTest, StringOperations) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test default constructor
    Debug::string str1;
    EXPECT_TRUE(str1.empty());
    
    // Test constructor with size - should trigger debug output
    Debug::string str2(5000, 'a'); // 5000 * 1 byte = 5KB > 1KB threshold
    EXPECT_EQ(str2.size(), 5000);
    EXPECT_EQ(str2[0], 'a');
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    EXPECT_TRUE(output.str().find("basic_string") != std::string::npos);
    
    // Test copy constructor
    output.str(""); // Clear output
    Debug::string str3 = str2; // Copying 5KB > 1KB threshold
    EXPECT_EQ(str3.size(), 5000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test resize
    output.str(""); // Clear output
    str1.resize(3000, 'b'); // 3000 * 1 byte = 3KB > 1KB threshold
    EXPECT_EQ(str1.size(), 3000);
    EXPECT_EQ(str1[0], 'b');
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test reserve
    output.str(""); // Clear output
    str1.reserve(4000); // 4000 * 1 byte = 4KB > 1KB threshold
    EXPECT_GE(str1.capacity(), 4000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test small allocation (should not trigger debug output)
    output.str(""); // Clear output
    Debug::string small_str(200, 'c'); // 200 * 1 byte = 200 bytes < 1KB threshold
    EXPECT_EQ(small_str.size(), 200);
    EXPECT_TRUE(output.str().empty()); // No debug output for small allocation
}

// Test map operations
TEST_F(DebugContainersTest, MapOperations) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test default constructor
    Debug::map<int, std::string> map1;
    EXPECT_TRUE(map1.empty());
    
    // Test copy constructor
    for (int i = 0; i < 1000; ++i) {
        map1[i] = "value" + std::to_string(i);
    }
    
    output.str(""); // Clear output
    Debug::map<int, std::string> map2 = map1; // Copying large map
    EXPECT_EQ(map2.size(), 1000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test assignment operator
    output.str(""); // Clear output
    Debug::map<int, std::string> map3;
    map3 = map1; // Assigning large map
    EXPECT_EQ(map3.size(), 1000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
}

// Test unordered_map operations
TEST_F(DebugContainersTest, UnorderedMapOperations) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test default constructor
    Debug::unordered_map<int, std::string> umap1;
    EXPECT_TRUE(umap1.empty());
    
    // Test copy constructor
    for (int i = 0; i < 800; ++i) {
        umap1[i] = "value" + std::to_string(i);
    }
    
    output.str(""); // Clear output
    Debug::unordered_map<int, std::string> umap2 = umap1; // Copying large map
    EXPECT_EQ(umap2.size(), 800);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test assignment operator
    output.str(""); // Clear output
    Debug::unordered_map<int, std::string> umap3;
    umap3 = umap1; // Assigning large map
    EXPECT_EQ(umap3.size(), 800);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test reserve
    output.str(""); // Clear output
    umap1.reserve(2000); // Should trigger debug output
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
}

// Test set operations
TEST_F(DebugContainersTest, SetOperations) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test default constructor
    Debug::set<int> set1;
    EXPECT_TRUE(set1.empty());
    
    // Test copy constructor
    for (int i = 0; i < 1000; ++i) {
        set1.insert(i);
    }
    
    output.str(""); // Clear output
    Debug::set<int> set2 = set1; // Copying large set
    EXPECT_EQ(set2.size(), 1000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test assignment operator
    output.str(""); // Clear output
    Debug::set<int> set3;
    set3 = set1; // Assigning large set
    EXPECT_EQ(set3.size(), 1000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
}

// Test unordered_set operations
TEST_F(DebugContainersTest, UnorderedSetOperations) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test default constructor
    Debug::unordered_set<int> uset1;
    EXPECT_TRUE(uset1.empty());
    
    // Test copy constructor
    for (int i = 0; i < 800; ++i) {
        uset1.insert(i);
    }
    
    output.str(""); // Clear output
    Debug::unordered_set<int> uset2 = uset1; // Copying large set
    EXPECT_EQ(uset2.size(), 800);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test assignment operator
    output.str(""); // Clear output
    Debug::unordered_set<int> uset3;
    uset3 = uset1; // Assigning large set
    EXPECT_EQ(uset3.size(), 800);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test reserve
    output.str(""); // Clear output
    uset1.reserve(2000); // Should trigger debug output
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
}

// Test list operations
TEST_F(DebugContainersTest, ListOperations) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test default constructor
    Debug::list<int> list1;
    EXPECT_TRUE(list1.empty());
    
    // Test constructor with size - should trigger debug output
    Debug::list<int> list2(2000); // 2000 * 4 bytes = 8KB > 1KB threshold
    EXPECT_EQ(list2.size(), 2000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test constructor with size and value
    output.str(""); // Clear output
    Debug::list<int> list3(1500, 42); // 1500 * 4 bytes = 6KB > 1KB threshold
    EXPECT_EQ(list3.size(), 1500);
    EXPECT_EQ(list3.front(), 42);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test copy constructor
    output.str(""); // Clear output
    Debug::list<int> list4 = list2; // Copying 8KB > 1KB threshold
    EXPECT_EQ(list4.size(), 2000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test assignment operator
    output.str(""); // Clear output
    Debug::list<int> list5;
    list5 = list3; // Assigning 6KB > 1KB threshold
    EXPECT_EQ(list5.size(), 1500);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
}

// Test deque operations
TEST_F(DebugContainersTest, DequeOperations) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test default constructor
    Debug::deque<int> deque1;
    EXPECT_TRUE(deque1.empty());
    
    // Test constructor with size - should trigger debug output
    Debug::deque<int> deque2(2000); // 2000 * 4 bytes = 8KB > 1KB threshold
    EXPECT_EQ(deque2.size(), 2000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test constructor with size and value
    output.str(""); // Clear output
    Debug::deque<int> deque3(1500, 42); // 1500 * 4 bytes = 6KB > 1KB threshold
    EXPECT_EQ(deque3.size(), 1500);
    EXPECT_EQ(deque3.front(), 42);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test copy constructor
    output.str(""); // Clear output
    Debug::deque<int> deque4 = deque2; // Copying 8KB > 1KB threshold
    EXPECT_EQ(deque4.size(), 2000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test assignment operator
    output.str(""); // Clear output
    Debug::deque<int> deque5;
    deque5 = deque3; // Assigning 6KB > 1KB threshold
    EXPECT_EQ(deque5.size(), 1500);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test resize
    output.str(""); // Clear output
    deque1.resize(2500); // 2500 * 4 bytes = 10KB > 1KB threshold
    EXPECT_EQ(deque1.size(), 2500);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test resize with value
    output.str(""); // Clear output
    deque1.resize(3000, 99); // 3000 * 4 bytes = 12KB > 1KB threshold
    EXPECT_EQ(deque1.size(), 3000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
}

// Test multiset operations
TEST_F(DebugContainersTest, MultisetOperations) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test default constructor
    Debug::multiset<int> mset1;
    EXPECT_TRUE(mset1.empty());
    
    // Test copy constructor
    for (int i = 0; i < 1000; ++i) {
        mset1.insert(i);
    }
    
    output.str(""); // Clear output
    Debug::multiset<int> mset2 = mset1; // Copying large multiset
    EXPECT_EQ(mset2.size(), 1000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test assignment operator
    output.str(""); // Clear output
    Debug::multiset<int> mset3;
    mset3 = mset1; // Assigning large multiset
    EXPECT_EQ(mset3.size(), 1000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
}

// Test unordered_multiset operations
TEST_F(DebugContainersTest, UnorderedMultisetOperations) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test default constructor
    Debug::unordered_multiset<int> umset1;
    EXPECT_TRUE(umset1.empty());
    
    // Test copy constructor
    for (int i = 0; i < 800; ++i) {
        umset1.insert(i);
    }
    
    output.str(""); // Clear output
    Debug::unordered_multiset<int> umset2 = umset1; // Copying large multiset
    EXPECT_EQ(umset2.size(), 800);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test assignment operator
    output.str(""); // Clear output
    Debug::unordered_multiset<int> umset3;
    umset3 = umset1; // Assigning large multiset
    EXPECT_EQ(umset3.size(), 800);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test reserve
    output.str(""); // Clear output
    umset1.reserve(2000); // Should trigger debug output
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
}

// Test multimap operations
TEST_F(DebugContainersTest, MultimapOperations) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test default constructor
    Debug::multimap<int, std::string> mmap1;
    EXPECT_TRUE(mmap1.empty());
    
    // Test copy constructor
    for (int i = 0; i < 1000; ++i) {
        mmap1.insert({i, "value" + std::to_string(i)});
    }
    
    output.str(""); // Clear output
    Debug::multimap<int, std::string> mmap2 = mmap1; // Copying large multimap
    EXPECT_EQ(mmap2.size(), 1000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test assignment operator
    output.str(""); // Clear output
    Debug::multimap<int, std::string> mmap3;
    mmap3 = mmap1; // Assigning large multimap
    EXPECT_EQ(mmap3.size(), 1000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
}

// Test unordered_multimap operations
TEST_F(DebugContainersTest, UnorderedMultimapOperations) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test default constructor
    Debug::unordered_multimap<int, std::string> ummap1;
    EXPECT_TRUE(ummap1.empty());
    
    // Test copy constructor
    for (int i = 0; i < 800; ++i) {
        ummap1.insert({i, "value" + std::to_string(i)});
    }
    
    output.str(""); // Clear output
    Debug::unordered_multimap<int, std::string> ummap2 = ummap1; // Copying large multimap
    EXPECT_EQ(ummap2.size(), 800);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test assignment operator
    output.str(""); // Clear output
    Debug::unordered_multimap<int, std::string> ummap3;
    ummap3 = ummap1; // Assigning large multimap
    EXPECT_EQ(ummap3.size(), 800);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test reserve
    output.str(""); // Clear output
    ummap1.reserve(2000); // Should trigger debug output
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
}

// Test queue operations
TEST_F(DebugContainersTest, QueueOperations) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test default constructor
    Debug::queue<int> queue1;
    EXPECT_TRUE(queue1.empty());
    
    // Test copy constructor
    for (int i = 0; i < 1000; ++i) {
        queue1.push(i);
    }
    
    output.str(""); // Clear output
    Debug::queue<int> queue2 = queue1; // Copying large queue
    EXPECT_EQ(queue2.size(), 1000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test assignment operator
    output.str(""); // Clear output
    Debug::queue<int> queue3;
    queue3 = queue1; // Assigning large queue
    EXPECT_EQ(queue3.size(), 1000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
}

// Test stack operations
TEST_F(DebugContainersTest, StackOperations) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test default constructor
    Debug::stack<int> stack1;
    EXPECT_TRUE(stack1.empty());
    
    // Test copy constructor
    for (int i = 0; i < 1000; ++i) {
        stack1.push(i);
    }
    
    output.str(""); // Clear output
    Debug::stack<int> stack2 = stack1; // Copying large stack
    EXPECT_EQ(stack2.size(), 1000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test assignment operator
    output.str(""); // Clear output
    Debug::stack<int> stack3;
    stack3 = stack1; // Assigning large stack
    EXPECT_EQ(stack3.size(), 1000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
}

// Test priority_queue operations
TEST_F(DebugContainersTest, PriorityQueueOperations) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test default constructor
    Debug::priority_queue<int> pq1;
    EXPECT_TRUE(pq1.empty());
    
    // Test copy constructor
    for (int i = 0; i < 1000; ++i) {
        pq1.push(i);
    }
    
    output.str(""); // Clear output
    Debug::priority_queue<int> pq2 = pq1; // Copying large priority queue
    EXPECT_EQ(pq2.size(), 1000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
    
    // Test assignment operator
    output.str(""); // Clear output
    Debug::priority_queue<int> pq3;
    pq3 = pq1; // Assigning large priority queue
    EXPECT_EQ(pq3.size(), 1000);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
}

// Test threshold configuration
TEST_F(DebugContainersTest, ThresholdConfiguration) {
    // Test default threshold
    size_t default_threshold = Debug::get_memory_threshold();
    EXPECT_EQ(default_threshold, Debug::DEFAULT_MEMORY_THRESHOLD);
    
    // Test setting custom threshold
    Debug::set_memory_threshold(5000); // 5KB
    EXPECT_EQ(Debug::get_memory_threshold(), 5000);
    
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test allocation below threshold (should not trigger output)
    Debug::vector<int> small_vec(1000); // 1000 * 4 bytes = 4KB < 5KB threshold
    EXPECT_TRUE(output.str().empty());
    
    // Test allocation above threshold (should trigger output)
    output.str(""); // Clear output
    Debug::vector<int> large_vec(2000); // 2000 * 4 bytes = 8KB > 5KB threshold
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
}

// Test custom output streams
TEST_F(DebugContainersTest, CustomOutputStreams) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << "[CUSTOM] " << message << std::endl;
    });
    
    // Test custom output
    Debug::vector<int> vec(5000); // Should trigger debug output
    EXPECT_TRUE(output.str().find("[CUSTOM]") != std::string::npos);
    EXPECT_TRUE(output.str().find("Large allocation detected") != std::string::npos);
}

// Test allocator functionality
TEST_F(DebugContainersTest, AllocatorFunctionality) {
    std::stringstream output;
    Debug::set_output_stream([&output](const std::string& message) {
        output << message << std::endl;
    });
    
    // Test that the allocator works correctly
    Debug::vector<int> vec;
    vec.push_back(42);
    vec.push_back(100);
    
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], 42);
    EXPECT_EQ(vec[1], 100);
    
    // Test that small allocations don't trigger debug output
    EXPECT_TRUE(output.str().empty());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
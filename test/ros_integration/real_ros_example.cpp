#include <memory/container_debug/debug_containers.hpp>
#include <iostream>

// Uncomment the following line and include ros/console.h for real ROS usage:
// #include <ros/console.h>

// For this example, we'll create a mock ROS logging system
// In real usage, replace this with actual ROS includes and logging

namespace ros {
    namespace console {
        class Stream {
        public:
            Stream& operator<<(const std::string& message) {
                std::cout << message;
                return *this;
            }
            
            Stream& operator<<(const char* message) {
                std::cout << message;
                return *this;
            }
            
            Stream& operator<<(int value) {
                std::cout << value;
                return *this;
            }
            
            Stream& operator<<(size_t value) {
                std::cout << value;
                return *this;
            }
            
            Stream& operator<<(double value) {
                std::cout << value;
                return *this;
            }
            
            Stream& operator<<(const Stream& (*manip)(Stream&)) {
                return manip(*this);
            }
        };
        
        Stream& endl(Stream& stream) {
            stream << "\n";
            return stream;
        }
        
        extern Stream ROS_WARN_STREAM;
        extern Stream ROS_ERROR_STREAM;
        extern Stream ROS_INFO_STREAM;
        extern Stream ROS_DEBUG_STREAM;
    }
}

// Mock ROS streams
ros::console::Stream ros::console::ROS_WARN_STREAM;
ros::console::Stream ros::console::ROS_ERROR_STREAM;
ros::console::Stream ros::console::ROS_INFO_STREAM;
ros::console::Stream ros::console::ROS_DEBUG_STREAM;

int main() {
    // Set memory threshold for demonstration
    Debug::set_memory_threshold(1024); // 1KB
    
    std::cout << "=== Real ROS Integration Example ===" << std::endl;
    std::cout << "Memory threshold: " << Debug::get_memory_threshold() << " bytes" << std::endl << std::endl;
    
    // Example 1: Basic ROS_WARN_STREAM integration
    std::cout << "1. Basic ROS_WARN_STREAM integration:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        ros::console::ROS_WARN_STREAM << "[MEMORY_DEBUG] " << message << ros::console::endl;
    });
    
    Debug::vector<int> vec1(500); // Should trigger ROS warning
    
    // Example 2: ROS_ERROR_STREAM for critical allocations
    std::cout << "\n2. ROS_ERROR_STREAM for critical allocations:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        // Check if allocation is very large (> 5KB)
        size_t pos = message.find("detected: ");
        if (pos != std::string::npos) {
            size_t end_pos = message.find(" bytes", pos);
            if (end_pos != std::string::npos) {
                std::string size_str = message.substr(pos + 10, end_pos - pos - 10);
                int size = std::stoi(size_str);
                
                if (size > 5000) {
                    ros::console::ROS_ERROR_STREAM << "[CRITICAL_MEMORY] " << message << ros::console::endl;
                } else {
                    ros::console::ROS_WARN_STREAM << "[MEMORY_WARNING] " << message << ros::console::endl;
                }
            }
        }
    });
    
    Debug::vector<int> small_vec(300);  // Should trigger WARNING
    Debug::vector<int> large_vec(1500); // Should trigger ERROR
    
    // Example 3: ROS_INFO_STREAM for informational logging
    std::cout << "\n3. ROS_INFO_STREAM for informational logging:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        ros::console::ROS_INFO_STREAM << "[MEMORY_INFO] " << message << ros::console::endl;
    });
    
    Debug::string str1(800, 'x'); // Should trigger ROS info
    
    // Example 4: ROS_DEBUG_STREAM for debug logging
    std::cout << "\n4. ROS_DEBUG_STREAM for debug logging:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        ros::console::ROS_DEBUG_STREAM << "[MEMORY_DEBUG] " << message << ros::console::endl;
    });
    
    Debug::map<int, std::string> map1;
    for (int i = 0; i < 200; ++i) {
        map1[i] = "value" + std::to_string(i);
    }
    
    // Example 5: Advanced ROS logging with node context
    std::cout << "\n5. Advanced ROS logging with node context:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        // Extract file and line information for better ROS logging
        size_t file_pos = message.find(" at ");
        if (file_pos != std::string::npos) {
            std::string file_info = message.substr(file_pos + 4);
            std::string allocation_info = message.substr(0, file_pos);
            
            ros::console::ROS_WARN_STREAM << "[NODE_MEMORY_DEBUG] " 
                                         << allocation_info 
                                         << " in file: " << file_info 
                                         << ros::console::endl;
        } else {
            ros::console::ROS_WARN_STREAM << "[NODE_MEMORY_DEBUG] " << message << ros::console::endl;
        }
    });
    
    Debug::unordered_map<int, std::string> umap1;
    umap1.reserve(600); // Should trigger advanced ROS logging
    
    // Example 6: ROS logging with different levels based on allocation size
    std::cout << "\n6. ROS logging with different levels based on allocation size:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        // Parse allocation size
        size_t pos = message.find("detected: ");
        if (pos != std::string::npos) {
            size_t end_pos = message.find(" bytes", pos);
            if (end_pos != std::string::npos) {
                std::string size_str = message.substr(pos + 10, end_pos - pos - 10);
                int size = std::stoi(size_str);
                
                if (size > 8000) {
                    ros::console::ROS_ERROR_STREAM << "[CRITICAL] " << message << ros::console::endl;
                } else if (size > 4000) {
                    ros::console::ROS_WARN_STREAM << "[WARNING] " << message << ros::console::endl;
                } else if (size > 2000) {
                    ros::console::ROS_INFO_STREAM << "[INFO] " << message << ros::console::endl;
                } else {
                    ros::console::ROS_DEBUG_STREAM << "[DEBUG] " << message << ros::console::endl;
                }
            }
        }
    });
    
    Debug::vector<int> tiny_vec(200);    // Should trigger DEBUG
    Debug::vector<int> small_vec2(500);  // Should trigger INFO
    Debug::vector<int> medium_vec(1000); // Should trigger WARNING
    Debug::vector<int> huge_vec(2000);   // Should trigger ERROR
    
    // Example 7: ROS logging with performance metrics
    std::cout << "\n7. ROS logging with performance metrics:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        // Add timestamp and performance context
        ros::console::ROS_WARN_STREAM << "[PERF_MEMORY] " 
                                     << "Time: " << std::time(nullptr) 
                                     << " | " << message 
                                     << " | Consider optimization" 
                                     << ros::console::endl;
    });
    
    Debug::list<int> list1(700); // Should trigger performance logging
    
    // Example 8: Reset to default for comparison
    std::cout << "\n8. Reset to default output for comparison:" << std::endl;
    Debug::set_output_to_cout();
    
    Debug::vector<int> vec2(400); // Should use default std::cout
    
    std::cout << "\n=== Real ROS Integration Example Completed ===" << std::endl;
    std::cout << "In real ROS usage, replace the mock ROS logging with:" << std::endl;
    std::cout << "#include <ros/console.h>" << std::endl;
    std::cout << "And use actual ROS logging macros." << std::endl;
    
    return 0;
}
#include <memory/container_debug/debug_containers.hpp>
#include <iostream>
#include <ctime>

// Simple ROS logging simulation
namespace ros {
    namespace console {
        void warn(const std::string& message) {
            std::cout << "[ROS_WARN] " << message << std::endl;
        }
        
        void error(const std::string& message) {
            std::cout << "[ROS_ERROR] " << message << std::endl;
        }
        
        void info(const std::string& message) {
            std::cout << "[ROS_INFO] " << message << std::endl;
        }
        
        void debug(const std::string& message) {
            std::cout << "[ROS_DEBUG] " << message << std::endl;
        }
    }
}

int main() {
    // Set memory threshold for demonstration
    Debug::set_memory_threshold(1000); // 1KB
    
    std::cout << "=== Simple ROS Integration Example ===" << std::endl;
    std::cout << "Memory threshold: " << Debug::get_memory_threshold() << " bytes" << std::endl << std::endl;
    
    // Example 1: Basic ROS_WARN integration
    std::cout << "1. Basic ROS_WARN integration:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        ros::console::warn(message);
    });
    
    Debug::vector<int> vec1(500); // Should trigger ROS warning
    
    // Example 2: ROS_ERROR for critical allocations
    std::cout << "\n2. ROS_ERROR for critical allocations:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        // Check if allocation is very large (> 5KB)
        size_t pos = message.find("detected: ");
        if (pos != std::string::npos) {
            size_t end_pos = message.find(" bytes", pos);
            if (end_pos != std::string::npos) {
                std::string size_str = message.substr(pos + 10, end_pos - pos - 10);
                int size = std::stoi(size_str);
                
                if (size > 5000) {
                    ros::console::error("CRITICAL: " + message);
                } else {
                    ros::console::warn("WARNING: " + message);
                }
            }
        }
    });
    
    Debug::vector<int> small_vec(300);  // Should trigger WARNING
    Debug::vector<int> large_vec(1500); // Should trigger ERROR
    
    // Example 3: ROS_INFO for informational logging
    std::cout << "\n3. ROS_INFO for informational logging:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        ros::console::info("MEMORY_INFO: " + message);
    });
    
    Debug::string str1(800, 'x'); // Should trigger ROS info
    
    // Example 4: ROS_DEBUG for debug logging
    std::cout << "\n4. ROS_DEBUG for debug logging:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        ros::console::debug("MEMORY_DEBUG: " + message);
    });
    
    Debug::map<int, std::string> map1;
    for (int i = 0; i < 200; ++i) {
        map1[i] = "value" + std::to_string(i);
    }
    
    // Example 5: Advanced ROS logging with different levels based on allocation size
    std::cout << "\n5. Advanced ROS logging with different levels:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        // Parse allocation size
        size_t pos = message.find("detected: ");
        if (pos != std::string::npos) {
            size_t end_pos = message.find(" bytes", pos);
            if (end_pos != std::string::npos) {
                std::string size_str = message.substr(pos + 10, end_pos - pos - 10);
                int size = std::stoi(size_str);
                
                if (size > 8000) {
                    ros::console::error("CRITICAL: " + message);
                } else if (size > 4000) {
                    ros::console::warn("WARNING: " + message);
                } else if (size > 2000) {
                    ros::console::info("INFO: " + message);
                } else {
                    ros::console::debug("DEBUG: " + message);
                }
            }
        }
    });
    
    Debug::vector<int> tiny_vec(200);    // Should trigger DEBUG
    Debug::vector<int> small_vec2(500);  // Should trigger INFO
    Debug::vector<int> medium_vec(1000); // Should trigger WARNING
    Debug::vector<int> huge_vec(2000);   // Should trigger ERROR
    
    // Example 6: ROS logging with timestamp
    std::cout << "\n6. ROS logging with timestamp:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        time_t now = time(nullptr);
        std::string timestamp = std::to_string(now);
        ros::console::warn("[" + timestamp + "] PERF_MEMORY: " + message + " - Consider optimization");
    });
    
    Debug::list<int> list1(700); // Should trigger timestamped logging
    
    // Example 7: Multiple ROS outputs
    std::cout << "\n7. Multiple ROS outputs:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        // Log to multiple ROS levels
        ros::console::warn("WARN: " + message);
        ros::console::info("INFO: " + message);
        ros::console::debug("DEBUG: " + message);
    });
    
    Debug::unordered_map<int, std::string> umap1;
    umap1.reserve(600); // Should trigger multiple outputs
    
    // Example 8: Reset to default for comparison
    std::cout << "\n8. Reset to default output:" << std::endl;
    Debug::set_output_to_cout();
    
    Debug::vector<int> vec2(400); // Should use default std::cout
    
    std::cout << "\n=== Simple ROS Integration Example Completed ===" << std::endl;
    std::cout << "In real ROS usage, replace the mock functions with:" << std::endl;
    std::cout << "#include <ros/console.h>" << std::endl;
    std::cout << "And use actual ROS logging macros like ROS_WARN_STREAM." << std::endl;
    
    return 0;
}
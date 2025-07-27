#include "debug_containers.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

// Mock ROS logging for demonstration (replace with actual ros/console.h in real usage)
namespace ros {
    class ConsoleStream {
    public:
        ConsoleStream& operator<<(const std::string& message) {
            std::cout << "[ROS_WARN] " << message << std::endl;
            return *this;
        }
        
        ConsoleStream& operator<<(const char* message) {
            std::cout << "[ROS_WARN] " << message << std::endl;
            return *this;
        }
        
        ConsoleStream& operator<<(int value) {
            std::cout << "[ROS_WARN] " << value << std::endl;
            return *this;
        }
    };
    
    static ConsoleStream ROS_WARN_STREAM;
    static ConsoleStream ROS_ERROR_STREAM;
    static ConsoleStream ROS_INFO_STREAM;
}

int main() {
    // Set a low threshold for demonstration
    Debug::set_memory_threshold(1000); // 1KB
    
    std::cout << "=== ROS Integration Example ===" << std::endl;
    std::cout << "Memory threshold: " << Debug::get_memory_threshold() << " bytes" << std::endl << std::endl;
    
    // Example 1: Default output (std::cout)
    std::cout << "1. Default output (std::cout):" << std::endl;
    Debug::vector<int> vec1(500); // Should trigger debug output
    
    // Example 2: Redirect to ROS_WARN_STREAM
    std::cout << "\n2. Redirecting to ROS_WARN_STREAM:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        ros::ROS_WARN_STREAM << message;
    });
    
    Debug::vector<int> vec2(600); // Should trigger ROS warning
    
    // Example 3: Redirect to ROS_ERROR_STREAM
    std::cout << "\n3. Redirecting to ROS_ERROR_STREAM:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        ros::ROS_ERROR_STREAM << message;
    });
    
    Debug::string str1(2000, 'x'); // Should trigger ROS error
    
    // Example 4: Redirect to ROS_INFO_STREAM
    std::cout << "\n4. Redirecting to ROS_INFO_STREAM:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        ros::ROS_INFO_STREAM << message;
    });
    
    Debug::map<int, std::string> map1;
    for (int i = 0; i < 300; ++i) {
        map1[i] = "value" + std::to_string(i);
    }
    
    // Example 5: Custom ROS logging with additional context
    std::cout << "\n5. Custom ROS logging with additional context:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        std::string enhanced_message = "[MEMORY_DEBUG] " + message + " - Check for memory leaks!";
        ros::ROS_WARN_STREAM << enhanced_message;
    });
    
    Debug::unordered_map<int, std::string> umap1;
    umap1.reserve(400); // Should trigger custom ROS warning
    
    // Example 6: File logging
    std::cout << "\n6. Redirecting to file:" << std::endl;
    std::ofstream log_file("memory_debug.log");
    Debug::set_output_to_file(log_file);
    
    Debug::vector<int> vec3(700); // Should write to file
    Debug::string str2(1500, 'y'); // Should write to file
    
    log_file.close();
    
    // Example 7: Multiple output streams
    std::cout << "\n7. Multiple output streams:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        // Log to ROS
        ros::ROS_WARN_STREAM << message;
        // Also log to file
        std::ofstream file("memory_debug.log", std::ios::app);
        file << "[FILE_LOG] " << message << std::endl;
        file.close();
        // And to console
        std::cout << "[CONSOLE] " << message << std::endl;
    });
    
    Debug::list<int> list1(800); // Should trigger multiple outputs
    
    // Example 8: Conditional logging based on allocation size
    std::cout << "\n8. Conditional logging based on allocation size:" << std::endl;
    Debug::set_output_stream([](const std::string& message) {
        // Extract size from message (simple parsing for demo)
        size_t pos = message.find("detected: ");
        if (pos != std::string::npos) {
            size_t end_pos = message.find(" bytes", pos);
            if (end_pos != std::string::npos) {
                std::string size_str = message.substr(pos + 10, end_pos - pos - 10);
                int size = std::stoi(size_str);
                
                if (size > 5000) {
                    ros::ROS_ERROR_STREAM << "CRITICAL: " << message;
                } else if (size > 2000) {
                    ros::ROS_WARN_STREAM << "WARNING: " << message;
                } else {
                    ros::ROS_INFO_STREAM << "INFO: " << message;
                }
            }
        }
    });
    
    Debug::vector<int> small_vec(300);  // Should trigger INFO
    Debug::vector<int> medium_vec(600); // Should trigger WARNING
    Debug::vector<int> large_vec(1500); // Should trigger ERROR
    
    // Example 9: Reset to default
    std::cout << "\n9. Reset to default output:" << std::endl;
    Debug::set_output_to_cout();
    
    Debug::vector<int> vec4(400); // Should use default std::cout
    
    std::cout << "\n=== ROS Integration Example Completed ===" << std::endl;
    std::cout << "Check 'memory_debug.log' file for file-based logging output." << std::endl;
    
    return 0;
}
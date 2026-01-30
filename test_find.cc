#include <iostream>
#include <string>

int main() {
    std::string s = "WARNING (file.cc:65) | TimeoutQueue destructor called without prior shutdown(). Invoking shutdown() automatically.";
    
    std::cout << "String: " << s << std::endl;
    std::cout << "Looking for 'Warning': " << s.find("Warning") << std::endl;
    std::cout << "Looking for 'WARNING': " << s.find("WARNING") << std::endl;
    std::cout << "npos value: " << std::string::npos << std::endl;
    
    if (s.find("Warning") != std::string::npos) {
        std::cout << "'Warning' found!" << std::endl;
    } else {
        std::cout << "'Warning' NOT found!" << std::endl;
    }
    
    if (s.find("WARNING") != std::string::npos) {
        std::cout << "'WARNING' found!" << std::endl;
    } else {
        std::cout << "'WARNING' NOT found!" << std::endl;
    }
    
    return 0;
}

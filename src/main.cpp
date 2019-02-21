#include <iostream>

#include "utilities/Logging.hpp"

//using namespace spdlog;

int main() {
    
    std::cout << "Hello c++17!" << std::endl;

    INFO("YOLO " << 12345);

}


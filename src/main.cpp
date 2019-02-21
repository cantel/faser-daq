#include <iostream>

#include "utilities/Logging.hpp"
#include "utilities/Common.hpp"

using namespace daq;

int main() {
    
    INFO("Hello c++17!");
    INFO("Kilo is: " << utilities::Constant::Kilo);
    INFO("ms is: " << utilities::ms );

    INFO("YOLO " << 12345);

}


#include <cstdlib>
#include <chrono>
#include <thread>

#include "core/Core.hpp"

using namespace std::chrono_literals;

int main(int argc, char **argv) {
    if (argc == 1)
    {
        ERROR("No command port provided!");
        return 1;
    }
    int port = atoi(argv[1]);
    INFO("Port " << port);
    daq::core::Core c(port, "tcp", "*");

    c.setupCommandPath();
    c.setupCommandHandler();

    while(!c.getShouldStop())
    {
        std::this_thread::sleep_for(1s);
    }

    return 0;
}


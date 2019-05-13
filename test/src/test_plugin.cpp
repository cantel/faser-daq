// enrico.gamberini@cern.ch

/// \cond
#include <dlfcn.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
/// \endcond

#include "Modules/BoardReader.hpp"
#include "Modules/EventBuilder.hpp"
#include "Modules/Monitor.hpp"
#include "Utilities/Logging.hpp"
#include "Core/ConnectionManager.hpp"

using namespace std::chrono_literals;

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        ERROR("No plugin name entered");
        return 1;
    }
    INFO("Loading " << argv[1]);
    std::string pluginName = "lib" + (std::string)argv[1] + ".so";
    void *handle = dlopen(pluginName.c_str(), RTLD_LAZY);
    if (handle == 0)
    {
        ERROR("Plugin name not valid");
        return 1;
    }

    DAQProcess *(*create)(...);
    void (*destroy)(DAQProcess *);

    create = (DAQProcess * (*)(...)) dlsym(handle, "create_object");
    destroy = (void (*)(DAQProcess *))dlsym(handle, "destroy_object");

    std::string name = "hello";
    int num = 42;
    
    DAQProcess *dp = (DAQProcess *)create(name, num);

    dp->start();
    std::this_thread::sleep_for(2s);
    dp->stop();
    destroy(dp);
}

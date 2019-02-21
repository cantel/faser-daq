// enrico.gamberini@cern.ch

#include <dlfcn.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include "modules/BoardReader.hpp"
#include "modules/EventBuilder.hpp"
#include "utilities/Logging.hpp"

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

    DAQProcess *(*create)(std::string name);
    void (*destroy)(DAQProcess *);

    create = (DAQProcess * (*)(std::string name)) dlsym(handle, "create_object");
    destroy = (void (*)(DAQProcess *))dlsym(handle, "destroy_object");

    DAQProcess *dp = (DAQProcess *)create("name");

    dp->start();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    dp->stop();
    destroy(dp);
}

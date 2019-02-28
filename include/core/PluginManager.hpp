// enrico.gamberini@cern.ch

#ifndef DAQ_CORE_PLUGINMANAGER_HPP
#define DAQ_CORE_PLUGINMANAGER_HPP

#include <dlfcn.h>
#include <string>
#include "modules/BoardReader.hpp"
#include "utilities/Logging.hpp"

namespace daq
{
namespace core
{

class PluginManager : public daq::utilities::Singleton<PluginManager>
{
  private:
    DAQProcess *(*m_create)(...);
    void (*m_destroy)(DAQProcess *);
    DAQProcess *m_dp;

  public:
    PluginManager();
    ~PluginManager();

    bool load(std::string name);
    void start() { m_dp->start(); };
    void stop() { m_dp->stop(); };
};

PluginManager::PluginManager()
    : m_create{},
      m_destroy{},
      m_dp{}
{
}

bool PluginManager::load(std::string name)
{
    std::string pluginName = "lib" + name + ".so";
    void *handle = dlopen(pluginName.c_str(), RTLD_LAZY);
    if (handle == 0)
    {
        ERROR("Plugin name not valid");
        return false;
    }

    m_create = (DAQProcess * (*)(...)) dlsym(handle, "create_object");
    m_destroy = (void (*)(DAQProcess *))dlsym(handle, "destroy_object");

    m_dp = (DAQProcess *)m_create();
}

PluginManager::~PluginManager()
{
    m_destroy(m_dp);
}

} // namespace core
} // namespace daq

#endif
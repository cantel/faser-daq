// enrico.gamberini@cern.ch

#ifndef DAQ_CORE_PLUGINMANAGER_HPP
#define DAQ_CORE_PLUGINMANAGER_HPP

/// \cond
#include <dlfcn.h>
#include <string>
/// \endcond

#include "core/DAQProcess.hpp"
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
    std::string getState() { return m_dp->getState(); }
    void setState(std::string state) { m_dp->setState(state); }
};

}
}

#endif

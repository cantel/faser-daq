// enrico.gamberini@cern.ch

#ifndef DAQ_CORE_PLUGINMANAGER_HPP
#define DAQ_CORE_PLUGINMANAGER_HPP

/// \cond
#include <dlfcn.h>
#include <string>
/// \endcond

#include "Core/DAQProcess.hpp"

#include "Utilities/Logging.hpp"

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
    void *m_handle;

  public:
    PluginManager();
    ~PluginManager();

    bool load(std::string name);
    void start() { m_dp->start(); };
    void stop() { m_dp->stop(); };
    std::string getState() { return m_dp->getState(); }
};

}  // namespace core
}  // namespace daq

#endif

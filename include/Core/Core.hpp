#ifndef DAQ_CORE_CORE_HPP_
#define DAQ_CORE_CORE_HPP_

#include "Utilities/Logging.hpp"
#include "Core/Configuration.hpp"
#include "Core/ConnectionManager.hpp"
#include "Core/PluginManager.hpp"
/// \cond
#include <atomic>
/// \endcond

namespace daq{
namespace core{

class Core
{
  public:
    Core(int port, std::string protocol, std::string address)
        : m_port{port}, m_protocol{protocol}, m_address{address} {};
    ~Core(){};

    bool setupCommandPath();
    bool setupCommandHandler();
    bool getShouldStop();

  private:
    int m_port;
    std::string m_protocol;
    std::string m_address;

    std::thread m_cmdHandler;

    // ZMQ ConnectionManager
    daq::core::ConnectionManager &m_connections = daq::core::ConnectionManager::instance();
    // Command exchange
    daq::core::Command &m_command = daq::core::Command::instance();
    // JSON Configuration map
    daq::core::Configuration &m_config = daq::core::Configuration::instance();
    // Plugin manager
    daq::core::PluginManager &m_plugin = daq::core::PluginManager::instance();
};

}
}

#endif


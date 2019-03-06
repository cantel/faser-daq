#ifndef DAQ_CORE_CORE_HPP_
#define DAQ_CORE_CORE_HPP_

#include "utilities/Logging.hpp"
#include "core/Configuration.hpp"
#include "core/ConnectionManager.hpp"
#include "core/PluginManager.hpp"

#include <atomic>

namespace daq{
namespace core{

class Core
{
  public:
    Core(int port, std::string protocol, std::string address)
        : m_port{port}, m_protocol{protocol}, m_address{address}, m_should_stop{false} {};
    ~Core(){};

    bool setupCommandPath();
    bool setupCommandHandler();
    // bool loadPlugin(const std::string& pluginName);
    bool getShouldStop() {return m_should_stop;};

  private:
    int m_port;
    std::string m_protocol;
    std::string m_address;

    std::thread m_cmdHandler;

    std::atomic<bool> m_should_stop;

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


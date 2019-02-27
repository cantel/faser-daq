#ifndef CORE_HPP_
#define CORE_HPP_

#include "utilities/Logging.hpp"
#include "core/Configuration.hpp"
#include "core/ConnectionManager.hpp"

#include <atomic>

class Core
{
  public:
    Core(int port, std::string protocol, std::string address)
        : m_port{port}, m_protocol{protocol}, m_address(address)
    {
        INFO(" BINDING COMMAND SOCKET...");
        std::string connStr("tcp://*:" + std::to_string(m_port));
        m_connections.setupCmdConnection(1, connStr);
    };

    ~Core(){};

  private:
    std::string m_address;
    int m_port;
    std::string m_protocol;

    std::thread m_cmdHandler;

    // ZMQ ConnectionManager
    daq::core::ConnectionManager &m_connections = daq::core::ConnectionManager::instance();
    // Command exchange
    daq::core::Command &m_command = daq::core::Command::instance();
    // JSON Configuration map
    daq::core::Configuration &m_config = daq::core::Configuration::instance();
};

#endif


#include "utilities/Logging.hpp"
#include "utilities/Common.hpp"

#include "core/Core.hpp"

#include <ctime>
#include <iomanip>
#include <thread>
#include <chrono>

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace daq::core;
using namespace std::chrono_literals;

bool Core::setupCommandPath(){
  INFO(" BINDING COMMAND SOCKET...");
  std::string connStr(m_protocol + "://" + m_address + std::to_string(m_port));
  m_connections.setupCommandConnection(1, connStr);
}

bool Core::loadPlugin(const std::string& pluginName) {
  m_plugin.load(pluginName);
}

bool Core::spawnCommandHandler(){
  return false;
}
 

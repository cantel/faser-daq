/// \cond
#include <chrono>
#include <iomanip>
/// \endcond

#include "Core/Core.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace daq::core;
using namespace std::chrono_literals;

bool Core::setupCommandPath() {
  std::string connStr(m_protocol + "://" + m_address + ":" + std::to_string(m_port));
  INFO(" BINDING COMMAND SOCKET : " << connStr);
  bool rv = m_connections.setupCommandConnection(1, connStr);
  return rv;
}

bool Core::setupCommandHandler() {
  m_command.startCommandHandler();
  return true;
}

bool Core::getShouldStop() { return m_command.getShouldStop(); }

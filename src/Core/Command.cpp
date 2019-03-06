#include "utilities/Logging.hpp"
#include "utilities/Common.hpp"
#include "core/Configuration.hpp"
#include "core/Command.hpp"
#include "core/PluginManager.hpp"

#include <ctime>
#include <iomanip>
#include <thread>
#include <chrono>

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace daq::core;
using namespace std::chrono_literals;

bool daq::core::Command::startCommandHandler()
{
  //m_commandHandler = std::make_unique<daq::utilities::ReusableThread>(10);
  unsigned tid = 1;
  m_commandFunctors.push_back(
      [&, tid] {
        INFO("CommandThread  ->>> Should handle message: " << m_message);
        std::string response;
        executeCommand(response);
        setResponse(response);
        setHandled(true);
      });
}

bool daq::core::Command::executeCommand(std::string& response)
{
  Configuration &cfg = Configuration::instance();
  cfg.load(m_message);
  INFO("Loaded configuration");
  auto command = cfg.get<std::string>("command");
  INFO("Get command: " << command );
  daq::core::PluginManager &m_plugin = daq::core::PluginManager::instance();
  if(command == "configure")
  {
    auto type = cfg.get<std::string>("type");
    INFO("Loading type: " << type);
    m_plugin.load(type);
    response = "Success";
    m_plugin.setState("ready");
  }
  else if(command == "start")
  {
    m_plugin.start();
    response = "Success";
    m_plugin.setState("running");
  }
  else if(command == "stop")
  {
    m_plugin.stop();
    response = "Success";
    m_plugin.setState("ready");
  }
  else if(command == "status")
  {
    response = m_plugin.getState();
  }
}

bool daq::core::Command::handleCommand()
{
  m_commandHandler->set_work(m_commandFunctors[0]);
  while (busy())
  {
    std::this_thread::sleep_for(500ms);
  }
  return true;
}

bool daq::core::Command::busy()
{
  bool busy = (m_commandHandler->get_readiness() == false) ? true : false;
  return busy;
}

//template <typename TValue, typename TPred>
//BinarySearchTree<TValue, TPred>::BinarySearchTree()

/*
template <class ST>
ConnectionManager<ST>::ConnectionManager(m_token)
{
  
}
*/

/*
template <class ST>
ConnectionManager<ST>::~ConnectionManager() {

}
*/

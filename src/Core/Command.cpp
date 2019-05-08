/// \cond
#include <chrono>
#include <ctime>
#include <iomanip>
#include <thread>
/// \endcond

#include "Core/Command.hpp"
#include "Core/Configuration.hpp"
#include "Core/ConnectionManager.hpp"
#include "Core/PluginManager.hpp"
#include "Utilities/Common.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace daq::core;
using namespace std::chrono_literals;

bool daq::core::Command::startCommandHandler() {
  // m_commandHandler = std::make_unique<daq::utilities::ReusableThread>(10);
  unsigned tid = 1;
  bool rv = false;
  m_commandFunctors.push_back([&, tid] {
    DEBUG("CommandThread  ->>> Should handle message: " << m_message);
    std::string response;
    rv = executeCommand(response);
    setResponse(response);
    setHandled(true);
  });
  return rv;
}

bool daq::core::Command::executeCommand(std::string& response) {
  Configuration& cfg = Configuration::instance();
  cfg.load(m_message);
  // INFO("Loaded configuration");
  auto command = cfg.get<std::string>("command");
  // INFO("Get command: " << command);
  auto& m_plugin = daq::core::PluginManager::instance();
  auto& cm = daq::core::ConnectionManager::instance();

  if (command == "configure") {
    auto type = cfg.get<std::string>("type");
    INFO("Loading type: " << type);

    auto j = cfg.getConfig();
    auto srcs = j["connections"]["sources"];
    INFO("sources empty " << srcs.empty());
    for (auto& it : srcs) {
      INFO("key" << it);
      if (it["type"] == "pair") {
        cm.addChannel(it["chid"], ConnectionManager::EDirection::CLIENT, 0, it["host"], it["port"],
                      100, false);
      } else if (it["type"] == "pubsub") {
        cm.addChannel(it["chid"], ConnectionManager::EDirection::SUBSCRIBER, 0, it["host"],
                      it["port"], 100, false);
      } else {
        ERROR("Connection type not recognized!");
        response = "Failure";
        ERROR("Shutting down...");
        stop_and_notify();
      }
    }

    auto dests = j["connections"]["destinations"];
    INFO("destinations empty " << dests.empty());
    for (auto& it : dests) {
      INFO("key" << it);
      if (it["type"] == "pair") {
        cm.addChannel(it["chid"], ConnectionManager::EDirection::SERVER, 0, it["host"], it["port"],
                      100, false);
      } else if (it["type"] == "pubsub") {
        cm.addChannel(it["chid"], ConnectionManager::EDirection::PUBLISHER, 0, it["host"],
                      it["port"], 100, false);
      } else {
        ERROR("Connection type not recognized!");
        response = "Failure";
        ERROR("Shutting down...");
        stop_and_notify();
      }
    }

    bool rv = m_plugin.load(type);
    if (rv == true) {
      response = "Success";
    } else {
      response = "Failure";
      ERROR("Shutting down...");
      stop_and_notify();
    }
  } else if (command == "start") {
    cm.start();

    m_plugin.start();
    response = "Success";

    INFO("Started connection manager");
  } else if (command == "stop") {
    m_plugin.stop();

    cm.stop();

    response = "Success";
  } else if (command == "shutdown") {
    stop_and_notify();
    response = "Success";
  } else if (command == "status") {
    if (m_plugin.getLoaded()) {
      response = m_plugin.getState();
    } else {
      response = "booted";
    }
  }
  return true;  // TODO put some meaning or return void
}

bool daq::core::Command::handleCommand() {
  m_commandHandler->set_work(m_commandFunctors[0]);
  while (busy()) {
    std::this_thread::sleep_for(100ms);
  }
  return true;
}

bool daq::core::Command::busy() {
  bool busy = (m_commandHandler->get_readiness() == false) ? true : false;
  return busy;
}

// template <typename TValue, typename TPred>
// BinarySearchTree<TValue, TPred>::BinarySearchTree()

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

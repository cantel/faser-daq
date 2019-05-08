/// \cond
#include <chrono>
#include <ctime>
#include <iomanip>
#include <thread>
/// \endcond

#include "Core/Command.hpp"
#include "Core/PluginManager.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace daq::core;
using namespace std::chrono_literals;

PluginManager::PluginManager() : m_create{}, m_destroy{}, m_dp{}, m_loaded{false} {}

PluginManager::~PluginManager() {
  if (m_handle != 0) {
    m_destroy(m_dp);
    m_loaded = false;
  }
}

bool PluginManager::load(std::string name) {
  std::string pluginName = "lib" + name + ".so";
  m_handle = dlopen(pluginName.c_str(), RTLD_LAZY);
  if (m_handle == 0) {
    ERROR("Plugin not loaded!");
    return false;
  }

  m_create = (DAQProcess * (*)(...)) dlsym(m_handle, "create_object");
  m_destroy = (void (*)(DAQProcess *))dlsym(m_handle, "destroy_object");

  m_dp = (DAQProcess *)m_create();
  m_loaded = true;
  return true;
}

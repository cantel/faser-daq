#include "utilities/Logging.hpp"
#include "utilities/Common.hpp"

#include "core/PluginManager.hpp"
#include "core/Command.hpp"

#include <ctime>
#include <iomanip>
#include <thread>
#include <chrono>

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace daq::core;
using namespace std::chrono_literals;



PluginManager::PluginManager()
    : m_create{},
      m_destroy{},
      m_dp{}
{
}

bool PluginManager::load(std::string name)
{
    std::string pluginName = "lib" + name + ".so";
    void *handle = dlopen(pluginName.c_str(), RTLD_LAZY);
    if (handle == 0)
    {
        ERROR("Plugin not loaded!");
        return false;
    }

    m_create = (DAQProcess * (*)(...)) dlsym(handle, "create_object");
    m_destroy = (void (*)(DAQProcess *))dlsym(handle, "destroy_object");

    m_dp = (DAQProcess *)m_create();
}

PluginManager::~PluginManager()
{
    m_destroy(m_dp);
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

 

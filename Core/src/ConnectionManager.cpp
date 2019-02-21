#include "utilities/Logging.hpp"
#include "core/ConnectionManager.hpp"

#include <ctime>
#include <iomanip>
#include <pthread.h>

#define REORD_DEBUG
#define QATCOMP_DEBUG

using namespace daq::core;

ConnectionManager::ConnectionManager(m_token)
{
  
}

ConnectionManager::~ConnectionManager() {
/*
  if (m_verbose) { 
    DAQLogger::LogInfo("ConnectionManager::~ConnectionManager")
      << "NIOH terminate ongoing... Stopping communication with FELIX."; 
  }  
*/
}



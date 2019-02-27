// enrico.gamberini@cern.ch

#ifndef DAQPROCESS_HPP_
#define DAQPROCESS_HPP_

// #include <atomic>

#include "utilities/Logging.hpp"
#include "utilities/Common.hpp"
#include "core/Configuration.hpp"
#include "core/ConnectionManager.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

class DAQProcess
{
  public:
    DAQProcess() { 
      INFO(__METHOD_NAME__ << " BINDING COMMAND SOCKET...");
      std::string connStr("tcp://188.185.65.114:5557");
      m_connections.setupCmdConnection(1, connStr);
    };

    virtual ~DAQProcess(){};

    /* use virtual otherwise linker will try to perform static linkage */
    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void runner() = 0;

  protected:
    // ZMQ ConnectionManager
    daq::core::ConnectionManager& m_connections = daq::core::ConnectionManager::instance(); 
    // JSON Configuration map
    daq::core::Configuration& m_config = daq::core::Configuration::instance();

};

#endif /* DAQPROCESS_HPP_ */

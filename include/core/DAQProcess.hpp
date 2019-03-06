// enrico.gamberini@cern.ch

#ifndef DAQPROCESS_HPP_
#define DAQPROCESS_HPP_

#include "utilities/Logging.hpp"
#include "utilities/Common.hpp"
#include "core/Configuration.hpp"
#include "core/ConnectionManager.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

class DAQProcess
{
  public:
    DAQProcess() {};

    virtual ~DAQProcess(){};

    /* use virtual otherwise linker will try to perform static linkage */
    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void runner() = 0;

    std::string getState() { return m_state; }

  protected:
    // ZMQ ConnectionManager
    daq::core::ConnectionManager& m_connections = daq::core::ConnectionManager::instance(); 
    // JSON Configuration map
    daq::core::Configuration& m_config = daq::core::Configuration::instance();

    std::string m_state;
};

#endif /* DAQPROCESS_HPP_ */

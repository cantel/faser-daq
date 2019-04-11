// enrico.gamberini@cern.ch

#ifndef DAQPROCESS_HPP_
#define DAQPROCESS_HPP_

#include "Utilities/Logging.hpp"
#include "Utilities/Common.hpp"
#include "Core/Configuration.hpp"
#include "Core/ConnectionManager.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

class DAQProcess
{
  public:
    DAQProcess() : m_state{""} {};

    virtual ~DAQProcess(){};

    /* use virtual otherwise linker will try to perform static linkage */
    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void runner() = 0;

    std::string getState() { return m_state; }
    void setState(std::string state) {
       m_state = state;
       if(m_state == "running") {
         m_run = true;
       }
       else if(m_state == "ready") {
         m_run = false;
       }
     }
    
  protected:
    // ZMQ ConnectionManager
    daq::core::ConnectionManager& m_connections = daq::core::ConnectionManager::instance(); 
    // JSON Configuration map
    daq::core::Configuration& m_config = daq::core::Configuration::instance();

    std::string m_state;
    std::atomic<bool> m_run;
    std::unique_ptr<std::thread> m_runner_thread;
};

#endif /* DAQPROCESS_HPP_ */

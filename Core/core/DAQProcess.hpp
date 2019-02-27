// enrico.gamberini@cern.ch

#ifndef DAQPROCESS_HPP_
#define DAQPROCESS_HPP_

// #include <atomic>

#include "core/Configuration.hpp"
#include "core/ConnectionManager.hpp"

class DAQProcess
{
  public:
    virtual ~DAQProcess(){};

    /* use virtual otherwise linker will try to perform static linkage */
    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void runner() = 0;

  protected:
    // std::atomic<bool> m_run;
    daq::core::Configuration& m_config = daq::core::Configuration::instance();

    // ZMQ ConnectionManager
    daq::core::ConnectionManager& m_connections = daq::core::ConnectionManager::instance(); 
};

#endif /* DAQPROCESS_HPP_ */

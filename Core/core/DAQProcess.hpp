// enrico.gamberini@cern.ch

#ifndef DAQPROCESS_HPP_
#define DAQPROCESS_HPP_

// #include <atomic>

#include "zmq.hpp"
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
    daq::core::ConnectionManager<zmq::context_t, zmq::socket_t>& m_connections = 
      daq::core::ConnectionManager<zmq::context_t, zmq::socket_t>::instance(); 
};

#endif /* DAQPROCESS_HPP_ */

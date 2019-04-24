// enrico.gamberini@cern.ch

#ifndef DAQPROCESS_HPP_
#define DAQPROCESS_HPP_

#include "Core/Configuration.hpp"
#include "Core/ConnectionManager.hpp"
#include "Utilities/Common.hpp"
#include "Utilities/Logging.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

class DAQProcess {
 public:
  DAQProcess() : m_state{"ready"} {};

  virtual ~DAQProcess(){};

  /* use virtual otherwise linker will try to perform static linkage */
  virtual void start() {
    m_run = true;
    m_runner_thread = std::make_unique<std::thread>(&DAQProcess::runner, this);
    m_state = "running";
  };

  virtual void stop() {
    m_run = false;
    m_runner_thread->join();
    m_state = "ready";
  };

  virtual void runner() = 0;

  std::string getState() { return m_state; }

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

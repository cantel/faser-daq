// enrico.gamberini@cern.ch

#ifndef DAQPROCESS_HPP_
#define DAQPROCESS_HPP_

// #include <atomic>

#include "utilities/configuration.hpp"

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
    daq::utilities::ProcessConfiguration& m_config = daq::utilities::ProcessConfiguration::instance();
};

#endif /* DAQPROCESS_HPP_ */

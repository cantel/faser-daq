// enrico.gamberini@cern.ch

#ifndef BASECLASS_HPP_
#define BASECLASS_HPP_

// #include <atomic>

#include "utilities/configuration.hpp"

class BaseClass
{
  public:
    virtual ~BaseClass(){};

    /* use virtual otherwise linker will try to perform static linkage */
    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void runner() = 0;

  protected:
    // std::atomic<bool> m_run;
    daq::utilities::ProcessConfiguration& m_config = daq::utilities::ProcessConfiguration::instance();
};

#endif /* BASECLASS_HPP_ */

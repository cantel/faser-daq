#ifndef DAQ_CORE_DATALOGGER_HPP_
#define DAQ_CORE_DATALOGGER_HPP_

#include "utilities/Logging.hpp"
#include "utilities/Common.hpp"
#include "core/Configuration.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

class DataLogger
{
  public:
    DataLogger() { }
    virtual ~DataLogger(){};

    virtual void setup() = 0;
    virtual void write() = 0;
    virtual void read() = 0;
    virtual void shutdown() = 0;

  protected:
    daq::core::Configuration& m_config = daq::core::Configuration::instance();

};

#endif /* DAQPROCESS_HPP_ */


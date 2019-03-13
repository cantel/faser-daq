#ifndef DAQ_MODULES_FILEDATALOGGER_HPP_
#define DAQ_MODULES_FILEDATALOGGER_HPP_

#include <iostream>

#include "core/DAQProcess.hpp"
#include "core/DataLogger.hpp"
#include "utilities/Binary.hpp"

class FileDataLogger : public DAQProcess, public DataLogger
{
  public:
    FileDataLogger();
    ~FileDataLogger();

    void start();
    void stop();
    void runner();

    void setup();
    void write();
    void read();
    bool write(uint64_t keyId, daq::utilities::Binary& payload);
    void shutdown();

};

#endif /* DAQ_MODULES_FILEDATALOGGER_HPP_ */

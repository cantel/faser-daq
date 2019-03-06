#ifndef DAQ_MODULES_FILEDATALOGGER_HPP_
#define DAQ_MODULES_FILEDATALOGGER_HPP_

#include <iostream>

#include "core/DAQProcess.hpp"
#include "core/DataLogger.hpp"

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
    void shutdown();

};

#endif /* DAQ_MODULES_FILEDATALOGGER_HPP_ */

#ifndef DAQ_MODULES_CASSANDRADATALOGGER_HPP_
#define DAQ_MODULES_CASSANDRADATALOGGER_HPP_

#include <iostream>

#include "core/DAQProcess.hpp"
#include "core/DataLogger.hpp"

class CassandraDataLogger : public DAQProcess, public DataLogger
{
  public:
    CassandraDataLogger();
    ~CassandraDataLogger();

    void start();
    void stop();
    void runner();

    void setup();
    void write();
    void read();
    void shutdown();

};

#endif /* DAQ_MODULES_CASSANDRADATALOGGER_HPP_ */


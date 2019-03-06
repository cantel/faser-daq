#ifndef DAQ_MODULES_CASSANDRADATALOGGER_HPP_
#define DAQ_MODULES_CASSANDRADATALOGGER_HPP_

#include <iostream>

#include "core/DAQProcess.hpp"
#include "core/DataLogger.hpp"

#include <cassandra.h>

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

  private:
    const std::string getErrorStr( CassFuture*& future );

    CassCluster* m_cluster;
    CassSession* m_session;

};

#endif /* DAQ_MODULES_CASSANDRADATALOGGER_HPP_ */


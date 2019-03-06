#ifndef NEWMODULE_H_
#define NEWMODULE_H_

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

#endif /* NEWMODULE_H_ */

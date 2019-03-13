// enrico.gamberini@cern.ch

#ifndef EVENTBUILDER_H_
#define EVENTBUILDER_H_

#include "core/DAQProcess.hpp"

class EventBuilder : public DAQProcess
{
  public:
    EventBuilder();
    ~EventBuilder();

    void start();
    void stop();

    void runner();
};

#endif /* EVENTBUILDER_H_ */

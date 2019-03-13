// enrico.gamberini@cern.ch

#ifndef EVENTBUILDER_H_
#define EVENTBUILDER_H_

#include <iostream>
#include <atomic>

#include "core/DAQProcess.hpp"

class EventBuilder : public DAQProcess
{
    std::atomic<bool> m_run;
    std::unique_ptr<std::thread> m_runner_thread;

  public:
    EventBuilder();
    ~EventBuilder();

    void start();
    void stop();

    void runner();
};

#endif /* EVENTBUILDER_H_ */

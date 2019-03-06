// enrico.gamberini@cern.ch

#ifndef BOARDREADER_HPP_
#define BOARDREADER_HPP_

#include <iostream>
#include <string>
#include <atomic>

#include "core/DAQProcess.hpp"

class BoardReader : public DAQProcess
{
    std::atomic<bool> m_run;
    std::unique_ptr<std::thread> m_runner_thread;

  public:
    BoardReader(std::string name, int num);
    ~BoardReader();
    void start();
    void stop();

    void runner();
};

#endif /* BOARDREADER_HPP_ */

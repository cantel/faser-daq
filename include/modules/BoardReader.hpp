// enrico.gamberini@cern.ch

#ifndef BOARDREADER_HPP_
#define BOARDREADER_HPP_

/// \cond
#include <string>
/// \endcond

#include "core/DAQProcess.hpp"

class BoardReader : public DAQProcess
{
  public:
    BoardReader(std::string name, int num);
    ~BoardReader();
    void start();
    void stop();

    void runner();
};

#endif /* BOARDREADER_HPP_ */

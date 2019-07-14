// enrico.gamberini@cern.ch

#ifndef EVENTBUILDER_H_
#define EVENTBUILDER_H_

#include "Core/DAQProcess.hpp"

class EventBuilder : public daqling::core::DAQProcess {
 public:
  EventBuilder();
  ~EventBuilder();

  void start();
  void stop();

  void runner();
private:
  unsigned int run_number;
};

#endif /* EVENTBUILDER_H_ */

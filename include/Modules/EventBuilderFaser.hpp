// enrico.gamberini@cern.ch

#ifndef EVENTBUILDER_H_
#define EVENTBUILDER_H_
#include <vector>

#include "Core/DAQProcess.hpp"

class EventBuilder : public daqling::core::DAQProcess {
 public:
  EventBuilder();
  ~EventBuilder();

  void start();
  void stop();
  void runner();
  bool sendEvent(int outChannel, std::vector<daqling::utilities::Binary *>& fragments, int numFragments);
private:
  unsigned int m_maxPending;
  unsigned int run_number;
  std::atomic<int> m_physicsEventCount;
  std::atomic<int> m_monitoringEventCount;
};

#endif /* EVENTBUILDER_H_ */

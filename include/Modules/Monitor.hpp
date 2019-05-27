#ifndef MONITOR_H_
#define MONITOR_H_

#include "Core/DAQProcess.hpp"

class Monitor : public daqling::core::DAQProcess {
 public:
  Monitor();
  ~Monitor();

  void start();
  void stop();

  void runner();
};

#endif /* MONITOR_H_ */

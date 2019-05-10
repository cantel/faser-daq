#ifndef TRIGGERGENERATOR_H_
#define TRIGGERGENERATOR_H_

#include <vector>

#include "Utilities/Udp.hpp"
#include "Core/DAQProcess.hpp"

class TriggerGenerator : public DAQProcess {
 public:
  TriggerGenerator();
  ~TriggerGenerator();

  void start();
  void stop();

  void runner();
private:
  unsigned int m_eventCounter;
  float m_rate;
  std::vector<UdpSender*> m_targets;
};

#endif /* TRIGGERGENERATOR_H_ */

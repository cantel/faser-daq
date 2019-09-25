#pragma once

#include <vector>

#include "Utils/Udp.hpp"
#include "Core/DAQProcess.hpp"

class TriggerGeneratorModule : public daqling::core::DAQProcess {
 public:
  TriggerGeneratorModule();
  ~TriggerGeneratorModule();

  void start();
  void stop();

  void runner();
private:
  unsigned int m_eventCounter;
  float m_rate;
  std::vector<UdpSender*> m_targets;
};

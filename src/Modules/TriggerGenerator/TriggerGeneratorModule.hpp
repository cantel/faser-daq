#pragma once

#include <vector>

#include "Utils/Udp.hpp"
#include "Commons/FaserProcess.hpp"

class TriggerGeneratorModule : public FaserProcess {
 public:
  TriggerGeneratorModule();
  ~TriggerGeneratorModule();

  void start(unsigned int);
  void enableTrigger(const std::string &arg);
  void disableTrigger(const std::string &arg);
  void stop();

  void runner();
private:
  unsigned int m_eventCounter;
  float m_rate;
  std::atomic<bool> m_enabled;
  std::vector<UdpSender*> m_targets;
};

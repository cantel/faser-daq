/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include <vector>

#include "Utils/Udp.hpp"
#include "Commons/FaserProcess.hpp"

class TriggerGeneratorModule : public FaserProcess {
 public:
  TriggerGeneratorModule(const std::string&);
  ~TriggerGeneratorModule();

  void configure();
  void start(unsigned int);
  void setRate(const std::string &arg);
  void enableTrigger(const std::string &arg);
  void disableTrigger(const std::string &arg);
  void stop();

  void runner() noexcept;
private:
  unsigned int m_eventCounter;
  std::atomic<int> m_triggerCount;
  std::atomic<int> m_rate;
  std::atomic<int> m_newrate;
  std::atomic<bool> m_enabled;
  std::vector<UdpSender*> m_targets;
};

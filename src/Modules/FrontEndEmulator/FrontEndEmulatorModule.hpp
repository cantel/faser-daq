/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include <chrono>
using namespace std::chrono_literals;
using namespace std::chrono;

#include "Commons/FaserProcess.hpp"
#include "Utils/Udp.hpp"
#include "EventFormats/RawExampleFormat.hpp"

class FrontEndEmulatorModule : public FaserProcess {
 public:
  FrontEndEmulatorModule(const std::string&);
  ~FrontEndEmulatorModule();

  void configure();
  void start(unsigned int);
  void stop();

  void runner() noexcept;
private:
  int m_meanSize;
  int m_rmsSize;
  uint32_t m_fragID;
  float m_probMissTrig;
  float m_probMissFrag;
  float m_probCorruptFrag;
  microseconds m_monitoringInterval;
  UdpReceiver m_trigIn;
  UdpSender m_outHandle;
  int m_eventCounter;
  microseconds m_timeMonitoring;
  MonitoringFragment m_monFrag;

  std::atomic<int> m_eventsSent;
  std::atomic<int> m_monitoringSent;

};

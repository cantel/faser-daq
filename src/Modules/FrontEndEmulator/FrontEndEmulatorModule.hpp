#pragma once

#include <chrono>
using namespace std::chrono_literals;
using namespace std::chrono;

#include "../Udp.hpp"
#include "Core/DAQProcess.hpp"
#include "../EventFormat.hpp"

class FrontEndEmulatorModule : public daqling::core::DAQProcess {
 public:
  FrontEndEmulatorModule();
  ~FrontEndEmulatorModule();

  void start();
  void stop();

  void runner();
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
};

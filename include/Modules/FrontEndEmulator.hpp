#ifndef FRONTENDEMULATOR_H_
#define FRONTENDEMULATOR_H_

#include <chrono>
using namespace std::chrono_literals;
using namespace std::chrono;

#include "Utilities/Udp.hpp"
#include "Core/DAQProcess.hpp"
#include "Modules/EventFormat.hpp"

class FrontEndEmulator : public daqling::core::DAQProcess {
 public:
  FrontEndEmulator();
  ~FrontEndEmulator();

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

#endif /* FRONTENDEMULATOR_H_ */

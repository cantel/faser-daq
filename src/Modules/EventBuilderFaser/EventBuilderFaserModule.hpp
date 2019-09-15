#pragma once

#include <vector>

#include "Core/DAQProcess.hpp"

enum StatusFlags { STATUS_OK=0,STATUS_WARN,STATUS_ERROR };

class EventBuilderFaserModule : public daqling::core::DAQProcess {
 public:
  EventBuilderFaserModule();
  ~EventBuilderFaserModule();

  void start();
  void stop();
  void runner();
  bool sendEvent(int outChannel, std::vector<daqling::utilities::Binary *>& fragments, int numFragments);
private:
  unsigned int m_maxPending;
  unsigned int m_numChannels;
  unsigned int m_numOutChannels;
  std::atomic<int> m_run_number;
  std::atomic<int> m_run_start;
  std::atomic<int> m_physicsEventCount;
  std::atomic<int> m_calibrationEventCount;
  std::atomic<int> m_monitoringEventCount;
  std::atomic<int> m_status;
  std::atomic<float> m_queueFraction;
};

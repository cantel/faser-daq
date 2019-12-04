#pragma once

#include <vector>

#include "Commons/FaserProcess.hpp"
#include "Commons/EventFormat.hpp"

enum StatusFlags { STATUS_OK=0,STATUS_WARN,STATUS_ERROR };

class EventBuilderFaserModule : public FaserProcess {
 public:
  EventBuilderFaserModule();
  ~EventBuilderFaserModule();

  void configure();
  void start(unsigned int run_num);
  void stop();
  void runner();
  bool sendEvent(uint8_t event_tag,
		 uint64_t event_number,
		 int outChannel,
		 std::vector<const EventFragment *>& fragments);

private:
  unsigned int m_maxPending;
  unsigned int m_numChannels;
  unsigned int m_numOutChannels;
  unsigned int m_timeout; //in milliseconds
  std::atomic<int> m_run_number;
  std::atomic<int> m_run_start;
  std::atomic<int> m_physicsEventCount;
  std::atomic<int> m_calibrationEventCount;
  std::atomic<int> m_monitoringEventCount;
  std::atomic<float> m_queueFraction;
  std::atomic<int> m_corruptFragmentCount;
  std::atomic<int> m_duplicateCount;
  std::atomic<int> m_duplicateSourceCount;
  std::atomic<int> m_overflowCount;
  std::atomic<int> m_timeoutCount;
  std::atomic<int> m_BCIDMismatchCount;
};

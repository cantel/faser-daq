/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include <vector>
#include <set>
#include <map>

#include "Commons/FaserProcess.hpp"
#include "EventFormats/DAQFormats.hpp"
#include "Exceptions/Exceptions.hpp"

using namespace DAQFormats;

enum StatusFlags { STATUS_OK=0,STATUS_WARN,STATUS_ERROR };

ERS_DECLARE_ISSUE(
EventBuilderFaser,                                                              // namespace
    EventBuilderIssue,                                                    // issue name
  message,  // message
    ((std::string) message)
)

class EventBuilderFaserModule : public FaserProcess {
 public:
  EventBuilderFaserModule(const std::string&);
  ~EventBuilderFaserModule();

  void configure();
  void start(unsigned int run_num);
  void stop();
  void runner() noexcept;
  bool sendEvent(uint8_t event_tag,EventFull *event);
  void addFragment(EventFragment *fragment);

private:
  unsigned int m_maxPending;
  unsigned int m_numChannels;
  unsigned int m_numOutChannels;
  unsigned int m_timeout; //in milliseconds
  unsigned int m_stopTimeout; //in milliseconds
  std::atomic<int> m_run_number;
  std::atomic<int> m_run_start;
  std::atomic<int> m_corruptFragmentCount;
  std::atomic<int> m_duplicateSourceCount;
  std::atomic<int> m_timeoutCount;
  std::atomic<int> m_BCIDMismatchCount;

  std::atomic<int> m_eventCounts[MaxAnyTag];
  std::atomic<int> m_pendingCounts[MaxAnyTag];
  std::atomic<int> m_sentCounts[MaxAnyTag];
  std::map<uint64_t,EventFull*> m_pendingEvents[MaxAnyTag];
  std::set<uint64_t> m_readyEvents[MaxAnyTag];
};

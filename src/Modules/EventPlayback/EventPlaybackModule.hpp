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

class EventPlaybackException : public Exceptions::BaseException { using Exceptions::BaseException::BaseException; };

class EventPlaybackModule : public FaserProcess {
 public:
  EventPlaybackModule(const std::string&);
  ~EventPlaybackModule();

  void configure();
  void start(unsigned int run_num);
  void stop();
  void runner() noexcept;
  bool sendEvent(uint8_t event_tag,EventFull *event);
  void addFragment(EventFragment *fragment);

private:
  microseconds m_timeBetween;
  unsigned int m_repeats;
  std::atomic<int> m_run_number;
  std::atomic<int> m_run_start;

  std::atomic<int> m_eventCounts[MaxAnyTag];
  
  std::vector<std::string> m_fileList;
};

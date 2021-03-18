/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#pragma once

#include "Commons/FaserProcess.hpp"
#include "TLBAccess/TLBAccess.h"

using namespace FASER;

class TriggerReceiverModule : public FaserProcess {
 public:
  TriggerReceiverModule();
  ~TriggerReceiverModule();

  void configure(); // optional (configuration can be handled in the constructor)
  void enableTrigger(const std::string &arg);
  void disableTrigger(const std::string &arg);
  void sendECR();
  void start(unsigned);
  void stop();

  void runner() noexcept;
  
 private:
 
  TLBAccess *m_tlb;
  
  bool m_enable_monitoringdata;
  bool m_enable_triggerdata;
 
  // metrics 
  std::atomic<int> m_physicsEventCount;
  std::atomic<int> m_monitoringEventCount;
  std::atomic<int> m_badFragmentsCount;
  std::atomic<int> m_trigger_payload_size;
  std::atomic<int> m_monitoring_payload_size;
  std::atomic<int> m_fragment_status;
  std::atomic<float> m_dataRate;
 
};

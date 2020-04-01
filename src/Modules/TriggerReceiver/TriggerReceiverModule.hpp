/**
 * Copyright (C) 2019 CERN
 * 
 * DAQling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * DAQling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with DAQling. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Commons/FaserProcess.hpp"
#include "TLBAccess/TLBAccess.h"
#include "TLBAccess/TLBDecode.h"

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

  void runner();
  
 private:
 
  FASER::TLBAccess *m_tlb;
  FASER::TLBDecode *m_decoder;
  
  std::atomic<int> m_physicsEventCount;
  std::atomic<int> m_monitoringEventCount;
  std::atomic<int> m_badFragmentsCount;
  std::atomic<int> m_trigger_payload_size;
  std::atomic<int> m_monitoring_payload_size;
  std::atomic<int> m_fragment_status;
 
};

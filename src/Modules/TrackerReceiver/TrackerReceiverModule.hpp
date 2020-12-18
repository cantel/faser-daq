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
#include "TrackerReadout/TRBAccess.h"
#include "TrackerReceiverModule.hpp" 
#include "GPIOBase/DummyInterface.h"
//#include "Commons/EventFormat.hpp"
//#include "Commons/RawExampleFormat.hpp"
#include "TrackerReadout/ConfigurationHandling.h"
#include <string>
#include <iostream>
#include <bitset>

class TrackerReceiverModule : public FaserProcess {
 public:
  TrackerReceiverModule();
  ~TrackerReceiverModule();

  void configure(); // optional (configuration can be handled in the constructor)
  void start(unsigned);
  void stop();
  void sendECR() override;
  void runner() noexcept;
  void disableTrigger(const std::string &) override;
  void enableTrigger(const std::string &) override;
  
 private:
  std::unique_ptr<FASER::TRBAccess> m_trb;
  unsigned int m_finePhaseDelay_Led0;
  unsigned int m_finePhaseDelay_Clk0;
  unsigned int m_hwDelay_Clk0;
  unsigned int m_finePhaseDelay_Led1;
  unsigned int m_finePhaseDelay_Clk1;
  unsigned int m_hwDelay_Clk1;
  unsigned int m_userBoardID;
  std::string m_SCIP;
  std::string m_DAQIP;
  bool m_extClkSelect;
  FASER::TRBAccess::ABCD_ReadoutMode m_ABCD_ReadoutMode;
  bool m_configureModules;
  bool m_RxTimeoutDisable;
  unsigned int m_moduleMask;
  unsigned int m_moduleClkCmdMask;
  bool m_triggerEnabled;

  std::atomic<int> m_event_size_bytes;
  std::atomic<int> m_physicsEventCount;
  std::atomic<int> m_corrupted_fragments;
  std::atomic<int> m_checksum_mismatches;
  std::atomic<float> m_checksum_mismatches_rate;
  std::atomic<int> m_number_of_decoded_events;

  bool m_debug = false;
  bool m_trace = false;
  const uint32_t m_TRBENDOFDAQ = 0x07000eee;
};

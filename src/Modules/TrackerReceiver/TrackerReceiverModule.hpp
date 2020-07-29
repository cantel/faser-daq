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
#include "TrackerReadout/TRBEventDecoder.h"
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
  void runner();
  void disableTrigger(const std::string &arg) override;
  void enableTrigger(const std::string &arg) override;
  
  std::unique_ptr<FASER::TRBAccess> m_trb;
  std::unique_ptr<FASER::TRBEventDecoder> m_ed;
  unsigned int m_userBoardID;
  unsigned int m_moduleMask;
  unsigned int m_moduleClkCmdMask;
  bool m_triggerEnabled;
  bool m_debug;

  std::atomic<int> event_size_bytes;
  std::atomic<int> event_id;
  std::atomic<int> bc_id;
  std::atomic<int> corrupted_fragments;
  std::atomic<int> m_physicsEventCount;
};

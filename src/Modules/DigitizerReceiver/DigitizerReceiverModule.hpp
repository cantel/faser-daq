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

// helper functions that do the actual communication with the digitizer
#include "Comm_vx1730.h" 

// needed for external tools
#include "Commons/FaserProcess.hpp"
#include "EventFormats/DAQFormats.hpp"
#include "Exceptions/Exceptions.hpp"

// needed for sendECR() and runner() protection
#include <mutex>

class DigitizerReceiverException : public Exceptions::BaseException { using Exceptions::BaseException::BaseException; };


class DigitizerReceiverModule : public FaserProcess {
 public:
  DigitizerReceiverModule();
  ~DigitizerReceiverModule();

  void configure(); // optional (configuration can be handled in the constructor)
  void start(unsigned int);
  void stop();
  void sendECR();
  void runner();

  void sendEvent();
  
  vx1730 *m_digitizer;
  
  std::mutex m_lock;
  
  // monitoring metrics
  std::atomic<int> m_triggers;

  // for SW trigger sending
  int m_sw_count;
  bool m_software_trigger_enable;
  float m_software_trigger_rate;
  
};

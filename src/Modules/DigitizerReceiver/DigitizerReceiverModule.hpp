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
#include "EventFormats/DAQFormats.hpp"

#include "Comm_vx1730.h"

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
  
  
};

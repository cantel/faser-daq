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

#include "TrackerReceiverModule.hpp"
#include "../../gpiodrivers/TRBAccess/TrackerReadout/TRBAccess.h"
//#include "../../gpiodrivers/GPIOBase/GPIOBase/GPIOBaseClass.h"

TRBAccess *trb = nullptr;

TrackerReceiverModule::TrackerReceiverModule() { INFO(""); }

TrackerReceiverModule::~TrackerReceiverModule() { INFO(""); }

// optional (configuration can be handled in the constructor)
void TrackerReceiverModule::configure() {
  FaserProcess::configure();
  INFO("");
}

void TrackerReceiverModule::start(unsigned run_num) {
  FaserProcess::start(run_num);
  INFO("");
}

void TrackerReceiverModule::stop() {
  FaserProcess::stop();
  INFO("");
}

void TrackerReceiverModule::runner() {
  INFO("Running...");
  while (m_run) {
  }
  INFO("Runner stopped");
}

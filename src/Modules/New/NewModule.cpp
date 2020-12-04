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

#include "NewModule.hpp"

NewModule::NewModule() { INFO(""); }

NewModule::~NewModule() { INFO(""); }

// optional (configuration can be handled in the constructor)
void NewModule::configure() {
  daqling::core::DAQProcess::configure();
  INFO("");
}

void NewModule::start(unsigned run_num) {
  daqling::core::DAQProcess::start(run_num);
  INFO("");
}

void NewModule::stop() {
  daqling::core::DAQProcess::stop();
  INFO("");
}

void NewModule::runner() noexcept {
  INFO("Running...");
  while (m_run) {
  }
  INFO("Runner stopped");
}

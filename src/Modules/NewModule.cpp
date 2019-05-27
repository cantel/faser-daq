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

#include "Modules/NewModule.hpp"

extern "C" NewModule *create_object() { return new NewModule; }
extern "C" void destroy_object(NewModule *object) { delete object; }

NewModule::NewModule() { INFO("NewModule::NewModule"); }

NewModule::~NewModule() { INFO("NewModule::~NewModule"); }

void NewModule::start() {
  daqling::core::DAQProcess::start();
  INFO("NewModule::start");
}

void NewModule::stop() {
  daqling::core::DAQProcess::stop();
  INFO("NewModule::stop");
}

void NewModule::runner() {
  INFO(" Running...");
  while (m_run) {
  }
  INFO(" Runner stopped");
}

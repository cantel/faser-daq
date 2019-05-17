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

#include "Modules/Test.hpp"

extern "C" Test *create_object() { return new Test; }

extern "C" void destroy_object(Test *object) { delete object; }

Test::Test() { INFO("Test::Test"); }

Test::~Test() { INFO("Test::~Test"); }

void Test::start() {
  daqling::core::DAQProcess::start();
  INFO("Test::start");
}

void Test::stop() {
  daqling::core::DAQProcess::stop();
  INFO("Test::stop");
}

void Test::runner() {
  INFO(" Running...");
  while (m_run) {
  }
  INFO(" Runner stopped");
}

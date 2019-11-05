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
#include "Commons/EventFormat.hpp"

#include "Comm_vx1730.h"

class DigitizerModule : public FaserProcess {
 public:
  DigitizerModule();
  ~DigitizerModule();

  void configure(); // optional (configuration can be handled in the constructor)
  void start(int);
  void stop();

  void runner();
  
  int m_var;
  
  vx1730 *m_digitizer;
  
  
};

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

#include "TriggerReceiverModule.hpp"


TriggerReceiverModule::TriggerReceiverModule() { INFO("TEST Ondrej"); }

TriggerReceiverModule::~TriggerReceiverModule() { INFO(""); }

// optional (configuration can be handled in the constructor)
void TriggerReceiverModule::configure() {
  FaserProcess::configure();
  
  INFO("TEST Eliott1");
  
  m_tlb = new FASER::TLBAccess();
  
  auto cfg = m_config.getSettings();
  
  
  std::cout<<"Configuring TLB"<<std::endl;
  if (m_tlb->ConfigureAndVerifyTLB(cfg)){std::cout<<"   TLB Configuration OK"<<std::endl;}
  else{ERROR("   Error: TLB Configuration failed");}
  std::cout << "   Done.\n"<<std::endl;

  
  INFO("TEST Eliott2");
}

void TriggerReceiverModule::start(unsigned run_num) {
  FaserProcess::start(run_num);
  INFO("TEST START");
}

void TriggerReceiverModule::stop() {
  FaserProcess::stop();
  INFO("");
}

void TriggerReceiverModule::runner() {
  INFO("Running...");
  while (m_run) {
    //gettlbdata
  }
  INFO("Runner stopped");
}

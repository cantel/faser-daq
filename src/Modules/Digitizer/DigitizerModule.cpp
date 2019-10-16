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

#include "DigitizerModule.hpp"

DigitizerModule::DigitizerModule() { INFO(""); 

  INFO("");

  auto cfg = m_config.getConfig()["settings"];
  
  m_var = cfg["myVar"];
  
  INFO("SAMSAMSAMSAM myVar printout "<<m_var);
  
  /*
  // ip address
  char  ip_addr_string[32];
  strcpy(ip_addr_string, std::string(cfg["ip"]).c_str() ) ; // SIS3153 IP address
  std::cout<<"\nIP Address : "<<ip_addr_string<<std::endl;

  // vme base address
  std::string vme_base_address_str = std::string(cfg["vme_base_address"]);
  UINT vme_base_address = std::stoi(vme_base_address_str,0,16);
  INFO("Base VME Address = 0x"<<std::setfill('0')<<std::setw(8)<<std::hex<<vme_base_address<<std::endl);

  // make a new digitizer instance
  vx1730 *digitizer = new vx1730(ip_addr_string, vme_base_address);

  // test digitizer board interface
  digitizer->TestComm();
*/
}

DigitizerModule::~DigitizerModule() { INFO(""); }

// optional (configuration can be handled in the constructor)
void DigitizerModule::configure() {
  daqling::core::DAQProcess::configure();
  INFO("");
}

void DigitizerModule::start() {
  daqling::core::DAQProcess::start();
  INFO("");
}

void DigitizerModule::stop() {
  daqling::core::DAQProcess::stop();
  INFO("");
}

void DigitizerModule::runner() {
  INFO("Running...");
  while (m_run) {
  }
  INFO("Runner stopped");
}

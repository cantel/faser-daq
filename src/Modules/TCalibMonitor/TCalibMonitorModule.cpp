/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

/// \cond
#include <chrono>
#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream
/// \endcond


#include "TCalibMonitorModule.hpp"
#include "Utils/Ers.hpp"

TCalibMonitorModule::TCalibMonitorModule(const std::string& n):FaserProcess(n) { ERS_INFO(""); }

TCalibMonitorModule::~TCalibMonitorModule() { ERS_INFO(""); }

// optional (configuration can be handled in the constructor)
void TCalibMonitorModule::configure() {
  FaserProcess::configure();
  ERS_INFO("");
  //registerCommand("foobar", "foobarring", "foobarred", &TCalibMonitorModule::foobar, this, _1);
  //which scan is filling
  //name histo accordingly
}

void TCalibMonitorModule::start(unsigned run_num) {
  FaserProcess::start(run_num);
  ERS_INFO("");
}

void TCalibMonitorModule::stop() {
  FaserProcess::stop();
  ERS_INFO("");
}

void TCalibMonitorModule::runner() noexcept {
  ERS_INFO("Running...");
  while (m_run) {
    //m_histogrammanager->fill("good_nhits_physics",goodHits) //one for high, one for low, x = strip id's
    //histogram per module; 12 chips x 127 channels = 1000-ish
    //higher and lower threshold bin for sure - could do instead: want hits per strip for each of the chips, per module.
    //    a histogram per module. then
    // on x = strip id. y axis: chip id 0-11. OR histo per chip. 8x12 histos for mask scans
    // display and analysis histos. display is low granulatrity
  }
  ERS_INFO("Runner stopped");
}

//--------------------------


//using namespace daqling::core;
//using namespace daqling::module;
//HistFiller::HistFiller(const std::string &n) : DAQProcess(n) { ERS_INFO(""); }
/*

void HistFiller::start(unsigned run_num) {
  daqling::core::DAQProcess::start(run_num);
  ERS_INFO("");
}

void HistFiller::stop() {
  daqling::core::DAQProcess::stop();
  ERS_INFO("");
}

void HistFiller::runner() noexcept {
  ERS_INFO("Running...");
  while (m_run) {
  }
  ERS_INFO("Runner stopped");
}


void TCalibMonitorModule::foobar(const std::string &arg) {
  ERS_INFO("Inside custom command. Got argument: " << arg);
}

*/

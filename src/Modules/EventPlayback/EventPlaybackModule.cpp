/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
/// \cond
#include <chrono>
/// \endcond

#include <map>
#include <algorithm>

#include "EventPlaybackModule.hpp"
#include <stdexcept>
#include <Utils/Binary.hpp>

using namespace std::chrono;
using namespace std::chrono_literals;

EventPlaybackModule::EventPlaybackModule(const std::string& n):FaserProcess(n) {
  auto cfg = m_config.getModuleSettings(getName());

  m_timeBetween = 1000000us/cfg.value("maxRate",10);
  m_repeats = cfg.value("repeats",1);
  for( auto& fileName : cfg["fileList"]) 
    m_fileList.push_back(fileName.get<std::string>());
}

EventPlaybackModule::~EventPlaybackModule() { }


void EventPlaybackModule::configure() {
  FaserProcess::configure();
  registerVariable(m_eventCounts[EventTags::PhysicsTag], "PhysicsEvents");
  registerVariable(m_eventCounts[EventTags::PhysicsTag], "PhysicsRate", metrics::RATE);
  registerVariable(m_eventCounts[EventTags::MonitoringTag], "MonitoringEvents");
  registerVariable(m_eventCounts[EventTags::MonitoringTag], "MonitoringRate", metrics::RATE);
  registerVariable(m_eventCounts[EventTags::TLBMonitoringTag], "TLBMonitoringEvents");
  registerVariable(m_eventCounts[EventTags::TLBMonitoringTag], "TLBMonitoringRate", metrics::RATE);
  registerVariable(m_eventCounts[EventTags::CalibrationTag], "CalibrationEvents");
  registerVariable(m_eventCounts[EventTags::CalibrationTag], "CalibrationRate", metrics::RATE);
  registerVariable(m_run_number, "RunNumber");
  registerVariable(m_run_start, "RunStart");
}

void EventPlaybackModule::start(unsigned int run_num) {
  FaserProcess::start(run_num);
  m_run_number = run_num; 
  m_run_start = std::time(nullptr);
  for(int ii=0;ii<MaxAnyTag;ii++) m_eventCounts[ii]=0;
  m_status = STATUS_OK;
}

void EventPlaybackModule::stop() {
  FaserProcess::stop();
}


bool EventPlaybackModule::sendEvent(uint8_t event_tag,EventFull *event) {
  int channel=event_tag; 
  INFO("Sending event "<<event->event_id()<<" - "<<event->size()<<" bytes on channel "<<channel);
  auto *bytestream=event->raw();
  DataFragment<daqling::utilities::Binary> binData(bytestream->data(),bytestream->size());
  m_connections.send(channel,binData);
  delete bytestream;
  return true;
}


void EventPlaybackModule::runner() noexcept {
  INFO("Running...");

  DataFragment<daqling::utilities::Binary>  blob;
  auto last=system_clock::now();
  std::ifstream infh;
  unsigned int runCount=0;
  auto curFileName=m_fileList.end();
  bool newFile=true;
  while (m_run) { 
    auto now=system_clock::now();
    microseconds delta=m_timeBetween-duration_cast<microseconds>(now-last);
    if (delta>0us) std::this_thread::sleep_for(delta);
    last=system_clock::now();
    if (newFile) {
      if (curFileName==m_fileList.end()) {
	runCount++;
	if (m_repeats!=0 && runCount>m_repeats) break;
	curFileName=m_fileList.begin();
      }
      infh.close();
      infh.open(*curFileName,std::ios::binary);
      if (!infh.is_open()) {
	ERROR("Failed to open file "<<*curFileName);
	break;
      }
      INFO("Sending events from "<<*curFileName);
      newFile=false;
    }
    
    if (infh.good() && infh.peek()!=EOF) {
      try {
	EventFull event(infh);
	uint8_t tag=event.event_tag();
	m_eventCounts[tag]++;
	sendEvent(tag,&event);
      } catch (EFormatException &e) {
	INFO("Got exception while reading "<<(*curFileName)<<":"<<e);
	newFile=true;
      }
    } else {
      newFile=true;
    }
    if (newFile) curFileName++;
  }
  INFO("Runner stopped");
}

/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
/// \cond
#include <chrono>
/// \endcond

#include <map>
#include <algorithm>

#include "EventBuilderFaserModule.hpp"
#include <stdexcept>
#include <Utils/Binary.hpp>

using namespace std::chrono;
using namespace std::chrono_literals;
using namespace EventBuilderFaser;
EventBuilderFaserModule::EventBuilderFaserModule(const std::string& n):FaserProcess(n) {
  auto cfg = getModuleSettings();

  m_maxPending = cfg.value("maxPending",10);
  m_timeout = 1000*cfg.value("timeout_ms",1000);
  m_stopTimeout = 1000*cfg.value("stopTimeout_ms",1000);
  m_numChannels=m_config.getNumReceiverConnections(getName());

}

EventBuilderFaserModule::~EventBuilderFaserModule() { }


void EventBuilderFaserModule::configure() {
  FaserProcess::configure();
  registerVariable(m_eventCounts[EventTags::PhysicsTag],       "Events_received_Physics");
  registerVariable(m_eventCounts[EventTags::PhysicsTag],       "Event_rate_Physics", metrics::RATE);
  //registerVariable(m_eventCounts[EventTags::MonitoringTag],    "Events_received_Monitoring");
  //registerVariable(m_eventCounts[EventTags::MonitoringTag],    "Event_rate_Monitoring", metrics::RATE);
  registerVariable(m_eventCounts[EventTags::TLBMonitoringTag], "Events_received_TLBMonitoring");
  registerVariable(m_eventCounts[EventTags::TLBMonitoringTag], "Event_rate_TLBMonitoring", metrics::RATE);
  registerVariable(m_eventCounts[EventTags::CalibrationTag],   "Events_received_Calibration");
  registerVariable(m_eventCounts[EventTags::CalibrationTag],   "Event_rate_Calibration", metrics::RATE);
  registerVariable(m_eventCounts[EventTags::CorruptedTag],     "Events_received_Corrupted");
  registerVariable(m_eventCounts[EventTags::CorruptedTag],     "Event_rate_Corrupted", metrics::RATE);
  registerVariable(m_pendingCounts[EventTags::PhysicsTag],     "Events_pending_Physics");
  //registerVariable(m_pendingCounts[EventTags::MonitoringTag], "Events_pending_Monitoring");
  //registerVariable(m_pendingCounts[EventTags::TLBMonitoringTag], "Events_pending_TLBMonitoring");
  //registerVariable(m_pendingCounts[EventTags::CalibrationTag], "Events_pending_Calibration");

  registerVariable(m_sentCounts[EventTags::PhysicsTag],         "Events_sent_Physics");
  //  registerVariable(m_sentCounts[EventTags::MonitoringTag],      "Events_sent_Monitoring");
  registerVariable(m_sentCounts[EventTags::TLBMonitoringTag],   "Events_sent_TLBMonitoring");
  registerVariable(m_sentCounts[EventTags::CalibrationTag],     "Events_sent_Calibration");
  registerVariable(m_sentCounts[EventTags::CorruptedTag],     "Events_sent_Corrupted");
  registerVariable(m_sentCounts[EventTags::IncompleteTag],     "Events_sent_Incomplete");
  registerVariable(m_sentCounts[EventTags::DuplicateTag],     "Events_sent_Duplicate");

  registerVariable(m_run_number, "RunNumber");
  registerVariable(m_run_start, "RunStart");
  registerVariable(m_corruptFragmentCount, "CorruptFragmentErrors");
  registerVariable(m_duplicateSourceCount, "DuplicateSourceErrors");
  registerVariable(m_timeoutCount, "TimeoutEventErrors");
  registerVariable(m_BCIDMismatchCount, "BCIDMisMatches");
}

void EventBuilderFaserModule::start(unsigned int run_num) {
  FaserProcess::start(run_num);
  m_run_number = run_num; 
  m_run_start = std::time(nullptr);
  for(int ii=0;ii<MaxAnyTag;ii++) {
    m_eventCounts[ii]=0;
    m_pendingCounts[ii]=0;
    m_sentCounts[ii]=0;
  }
  m_corruptFragmentCount=0;
  m_duplicateSourceCount=0;
  m_timeoutCount=0;
  m_BCIDMismatchCount=0;
  m_status = STATUS_OK;
}

void EventBuilderFaserModule::stop() {
  std::this_thread::sleep_for(std::chrono::microseconds(m_stopTimeout)); //wait for events 
  FaserProcess::stop();
}


bool EventBuilderFaserModule::sendEvent(uint8_t event_tag,EventFull *event) {
  int channel=event_tag; 
  DEBUG("Sending event "<<event->event_id()<<" - "<<event->size()<<" bytes on channel "<<channel);
  auto *bytestream=event->raw();
  // could be optimized by using SharedDataType with EventFull as inner data
  DataFragment<daqling::utilities::Binary> binData(bytestream->data(),bytestream->size());
  DataFragment<daqling::utilities::Binary> binData1(bytestream->data(),bytestream->size());
  m_connections.send(channel,binData);      // to file writer
  m_connections.send(channel+100,binData1);  // to monitoring
  delete bytestream;
  return true;
}

void EventBuilderFaserModule::addFragment(EventFragment *fragment) {
  auto event_id=fragment->event_id();
  auto fragment_tag=fragment->fragment_tag();
  auto status=fragment->status();
  uint8_t event_tag=fragment_tag;
  
  if ((fragment_tag!=EventTags::CorruptedTag) &&
      (fragment_tag!=EventTags::DuplicateTag) &&
      (fragment_tag>EventTags::MaxRegularTag)) {
    event_tag=EventTags::CorruptedTag; //reroute unknown tags
  }
  
  if (status & EventStatus::CorruptedFragment) {
    m_corruptFragmentCount++;
    event_tag=EventTags::CorruptedTag; //reroute
  }

  auto& pendingEvents=m_pendingEvents[event_tag];
  if (pendingEvents.find(event_id)==pendingEvents.end()) {
    pendingEvents[event_id]=new EventFull(event_tag,m_run_number,++m_eventCounts[event_tag]);
  }

  auto event=pendingEvents[event_id];
  if (!event) throw EventBuilderIssue(ERS_HERE,"Out of memory");
  
  try {
    auto status=event->addFragment(fragment);
    if (status&EventStatus::BCIDMismatch) {
      if (abs(event->bc_id()-fragment->bc_id())>10) { //allow for digitizer to be slightly out of time
	WARNING("Mismatch in BCID for event "<<event->event_id()<<" : "<<event->bc_id()<<" != "<<fragment->bc_id());
	m_BCIDMismatchCount++;
      }
    }
  }
  catch (const std::runtime_error& e) {
    ERROR("Got error in fragment: "<<e.what());
    m_duplicateSourceCount++;
    if (event_tag!=EventTags::DuplicateTag) { //reroute to duplicate stream unless already tried that
      fragment->set_fragment_tag(EventTags::DuplicateTag);
      addFragment(fragment);
    } else {
      ERROR("Failed to transmit duplicate fragment");
      delete fragment; // just give up
    }
    return;
  }
  // for now hardcoded that only physics events have multiple fragments
  // anything else gets sent immediately
  if (((event_tag==EventTags::PhysicsTag) && (event->fragment_count()==m_numChannels)) ||
      (event_tag!=EventTags::PhysicsTag)) { 
    m_readyEvents[event_tag].insert(event_id);
  }
}



void EventBuilderFaserModule::runner() noexcept {
  INFO("Running...");

  bool noData=true;
  DataFragment<daqling::utilities::Binary>  blob;
  while (m_run) { 
    if (m_timeoutCount>10) m_status=STATUS_WARN;
    if (m_timeoutCount>100) m_status=STATUS_ERROR;

    noData=true;
    auto receivers = m_config.getConnections(m_name)["receivers"];
    for (auto receiver : receivers) {
      auto channel = receiver["chid"]; 
      if (m_connections.receive(channel, blob)) {
	noData=false;
	EventFragment* fragment;
	try {
	  fragment = new EventFragment(blob.data<uint8_t *>(),blob.size());
	} catch (const std::runtime_error& e) {
	  ERROR("Got error in fragment ("<<blob.size()<<" bytes) from channel "<<channel<<": "<<e.what());
	  m_corruptFragmentCount++;
	  fragment = new EventFragment(EventTags::CorruptedTag,channel,
				       0xFFFFFFFF,0xFFFF,
				       blob.data<void *>(),blob.size());
	  fragment->set_status(EventStatus::ErrorFragment);
	}
	addFragment(fragment);
      }
    }

    // send any events ready or timed out
    bool sentMissing=false;
    microseconds now;
    now = duration_cast<microseconds>(system_clock::now().time_since_epoch());
    for(unsigned int tag=0;tag<MaxAnyTag;tag++) {
      for( auto & event_id : m_readyEvents[tag]) {
	sendEvent(tag,m_pendingEvents[tag][event_id]);
	delete m_pendingEvents[tag][event_id];
	m_pendingEvents[tag].erase(event_id);
	m_sentCounts[tag]++;
      }
      m_readyEvents[tag].clear();
      // check oldest event
      m_pendingCounts[tag]=m_pendingEvents[tag].size();
      if (m_pendingEvents[tag].size()) {
	auto event=m_pendingEvents[tag].begin()->second;
	if ((now.count()-event->timestamp())>m_timeout) {
	  event->updateStatus(EventStatus::MissingFragment);
	  WARNING("Missing fragments for "<<event->event_id());
	  m_timeoutCount++;
	  sendEvent(EventTags::IncompleteTag,event);
	  delete event;
	  m_pendingEvents[tag].erase(m_pendingEvents[tag].begin());
	  m_sentCounts[EventTags::IncompleteTag]++;
	  sentMissing=true;
	}
      }
    }
    if (noData&&!sentMissing) std::this_thread::sleep_for(10ms);

  }
  INFO("Runner stopped");
}

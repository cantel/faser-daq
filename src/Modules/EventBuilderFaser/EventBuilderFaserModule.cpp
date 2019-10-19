/// \cond
#include <chrono>
/// \endcond

#include <map>
#include <algorithm>

#include "EventBuilderFaserModule.hpp"
#include <stdexcept>

using namespace std::chrono;
using namespace std::chrono_literals;

EventBuilderFaserModule::EventBuilderFaserModule() {
  INFO("With config: " << m_config.dump() << " getState: " << this->getState());
  auto cfg = m_config.getSettings();

  m_maxPending = cfg.value("maxPending",10);
  m_timeout = cfg.value("timeout_ms",1000);

  m_numChannels=m_config.getConnections()["receivers"].size();
  m_numOutChannels=m_config.getConnections()["senders"].size();

}

EventBuilderFaserModule::~EventBuilderFaserModule() { }

void EventBuilderFaserModule::start(int run_num) {
  DAQProcess::start(run_num);
  m_physicsEventCount = 0;
  m_calibrationEventCount = 0;
  m_monitoringEventCount = 0;
  m_run_number = run_num; 
  m_run_start = std::time(nullptr);
  m_status = STATUS_OK;
  m_queueFraction = 0;
  m_duplicateCount = 0;
  m_duplicateSourceCount = 0;
  m_overflowCount = 0;
  m_timeoutCount = 0;
  m_BCIDMismatchCount = 0;
  m_corruptFragmentCount = 0;
  if (m_stats_on) {
    m_statistics->registerVariable<std::atomic<size_t>, size_t>(&m_connections.getQueueStat(1), "EB-CHN1-QueueSizeGuess", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::SIZE);
    m_statistics->registerVariable<std::atomic<size_t>, size_t>(&m_connections.getMsgStat(1), "EB-CHN1-NumMessages", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::SIZE);
    m_statistics->registerVariable<std::atomic<size_t>, size_t>(&m_connections.getQueueStat(2), "EB-CHN2-QueueSizeGuess", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::SIZE);
    m_statistics->registerVariable<std::atomic<size_t>, size_t>(&m_connections.getMsgStat(2), "EB-CHN2-NumMessages", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::SIZE);
    // Claire: above metrics will in future be added automatically by daqling for every existing connection
    m_statistics->registerVariable<std::atomic<int>, int>(&m_physicsEventCount, "PhysicsEvents", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_physicsEventCount, "PhysicsRate", daqling::core::metrics::RATE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_monitoringEventCount, "MonitoringEvents", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_monitoringEventCount, "MonitoringRate", daqling::core::metrics::RATE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_calibrationEventCount, "CalibrationEvents", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_calibrationEventCount, "CalibrationRate", daqling::core::metrics::RATE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_run_number, "RunNumber", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_run_start, "RunStart", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_status, "Status", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<float>, float>(&m_queueFraction, "QueueFraction", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::FLOAT);

    m_statistics->registerVariable<std::atomic<int>, int>(&m_corruptFragmentCount, "CorruptFragmentErrors", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_duplicateCount, "DuplicateEventErrors", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_duplicateSourceCount, "DuplicateSourceErrors", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_overflowCount, "OverflowEventErrors", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_timeoutCount, "TimeoutEventErrors", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_BCIDMismatchCount, "BCIDMisMatches", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);
  }
  INFO("getState: " << getState());
}

void EventBuilderFaserModule::stop() {
  DAQProcess::stop();
  INFO("getState: " << this->getState());
}


bool EventBuilderFaserModule::sendEvent(uint8_t event_tag,
					uint64_t event_number,
					int outChannel,
					std::vector<const EventFragment *>& fragments) {
      daqling::utilities::Binary outFragments;
      EventFull event(event_tag,m_run_number,event_number);

      for(unsigned int ch=0;ch<fragments.size();ch++) {
	if (fragments.at(ch)) {
	  try {
	    auto status=event.addFragment(fragments[ch]);
	    if (status&EventStatus::BCIDMismatch) {
	      WARNING("Mismatch in BCID for event "<<event_number<<" : "<<event.bc_id()<<" != "<<fragments[ch]->bc_id());
	      m_BCIDMismatchCount++;
	    }
	  } catch (const std::runtime_error& e) {
	    ERROR("Got error in fragment: "<<e.what());
	    m_duplicateSourceCount++;
	  }
	} else {
	  WARNING("Missing fragment for event "<<event_number<<": No fragment for channel "<<ch);
	  event.updateStatus(EventStatus::MissingFragment);
	}
      }
      INFO("Sending event "<<event.event_id()<<" - "<<event.size()<<" bytes.");
      Binary* data=event.raw();
      m_connections.put(outChannel, *data);
      delete data;
      return true;
}


struct pendingEvent_t {
  time_point<steady_clock> start_time;
  unsigned int fragmentCount;
  std::vector<const EventFragment *> fragments;
};

void EventBuilderFaserModule::runner() {
  INFO("Running...");

  unsigned int channelNum=1;
  bool noData=true;
  std::map<uint64_t, pendingEvent_t> pendingEvents;
  daqling::utilities::Binary  blob;
  while (m_run) { //BP: At the moment there is no flushing of partial events on stop
    m_queueFraction=1.0*pendingEvents.size()/m_maxPending;
    if (m_queueFraction>0.5) { //BP: not a good example of status flag filling as nothing else is considered
      m_status=STATUS_WARN;
      if (m_queueFraction>0.85) {
	m_status=STATUS_ERROR;
      }
    } else {
      m_status=STATUS_OK;
    }
    
    int channel=channelNum;
    channelNum++;
    if (channelNum>m_numChannels) channelNum=1;
    
    if (!m_connections.get(channel, blob)) {
      if (noData && channelNum==1) std::this_thread::sleep_for(10ms);
      noData=true;
      continue;
    }
    noData=false;
    const EventFragment* fragment;
    try {
      fragment = new EventFragment(blob);
    } catch (const std::runtime_error& e) {
      ERROR("Got error in fragment ("<<blob.size()<<" bytes) from channel "<<channel<<": "<<e.what());
      INFO("Received:\n"<<blob);
      m_corruptFragmentCount++;
      continue;
    }
    auto event_id=fragment->event_id();
    auto fragment_tag=fragment->fragment_tag(); 
    if ((fragment_tag==MonitoringTag) || (fragment_tag==TLBMonitoringTag)) {
      INFO("Got monitoring fragment : "<<event_id<<" from channel "<<channel);
      //send monitoring fragments immediately as no other fragments are expected for thos
      std::vector<const EventFragment *> monFragmentList;
      monFragmentList.push_back(fragment);
      sendEvent(fragment_tag,m_monitoringEventCount,m_numChannels+1,monFragmentList);
      m_monitoringEventCount++;
      continue;
    }
 
    INFO("Got data fragment : "<<event_id<<" from channel "<<channel << " with size " << blob.size());
    if (pendingEvents.find(event_id)==pendingEvents.end()) { //got first fragment in a new event
      struct pendingEvent_t pending;
      pending.start_time=steady_clock::now();
      pending.fragmentCount=0;
      pending.fragments=std::vector<const EventFragment *>(m_numChannels,0);
      pendingEvents[event_id]=pending;
    }
    int ch=channel-1;
    if (pendingEvents[event_id].fragments[ch]!=0) { 
      //BP: this won't catch if an incomplete event was already sent
      ERROR("Got same event from same source more than once!");
      m_duplicateCount++;
    } else {
      pendingEvents[event_id].fragmentCount++;
    }
    pendingEvents[event_id].fragments[ch]=fragment;

    uint64_t eventToSend=0;
    if (pendingEvents[event_id].fragmentCount==m_numChannels) {
      eventToSend=event_id;
    } else if (pendingEvents.size()>m_maxPending) {
      eventToSend=pendingEvents.begin()->first;
      WARNING("Too many events pending - will send first incomplete event (EventID: "<<eventToSend<<")");
      m_overflowCount++;
    } else {
      auto oldestEvent=pendingEvents.begin()->second;
      if ( duration_cast<std::chrono::milliseconds>(steady_clock::now()-oldestEvent.start_time).count()>m_timeout) {
	eventToSend=pendingEvents.begin()->first;
	WARNING("Incomplete event timed out (EventID: "<<eventToSend<<")");
	m_timeoutCount++;
      }
    }
    
    if (eventToSend) {
      sendEvent(PhysicsTag,m_physicsEventCount,m_numChannels+1,pendingEvents[eventToSend].fragments);
      pendingEvents.erase(pendingEvents.find(eventToSend));
      m_physicsEventCount++;
    }
  }
  INFO("Runner stopped");
}

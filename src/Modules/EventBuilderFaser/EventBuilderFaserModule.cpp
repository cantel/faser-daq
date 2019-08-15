/// \cond
#include <chrono>
/// \endcond

#include <map>
#include <algorithm>

#include "EventBuilderFaserModule.hpp"
#include "../EventFormat.hpp"

using namespace std::chrono_literals;

EventBuilderFaserModule::EventBuilderFaserModule() {
  INFO("With config: " << m_config.dump() << " getState: " << this->getState());
  auto cfg = m_config.getConfig()["settings"];

  m_maxPending = cfg["maxPending"];

  m_numChannels=m_config.getConfig()["connections"]["receivers"].size();
  m_numOutChannels=m_config.getConfig()["connections"]["senders"].size();

}

EventBuilderFaserModule::~EventBuilderFaserModule() { }

void EventBuilderFaserModule::start() {
  DAQProcess::start();
  m_physicsEventCount = 0;
  m_calibrationEventCount = 0;
  m_monitoringEventCount = 0;
  m_run_number = 100; //BP: should get this from run control
  m_run_start = std::time(nullptr);
  m_status = STATUS_OK;
  m_queueFraction = 0;
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
  }
  INFO("getState: " << getState());
}

void EventBuilderFaserModule::stop() {
  DAQProcess::stop();
  INFO("getState: " << this->getState());
}


bool EventBuilderFaserModule::sendEvent(int outChannel,
			     std::vector<daqling::utilities::Binary *>& fragments,
			     int numFragments) {
      daqling::utilities::Binary outFragments;
      EventHeader header;
      header.marker = EventMarker;
      header.event_tag = PhysicsTag; //updated below
      header.trigger_bits = 0; //set below
      header.version_number = EventHeaderVersion;
      header.header_size = sizeof(header);
      header.payload_size = 0; //set below
      header.run_number = m_run_number;
      header.fragment_count = 0;
      header.event_id = 0;
      header.bc_id = 0xFFFF;
      header.status = 0;
      header.timestamp = 0;

      for(int ch=0;ch<numFragments;ch++) {
	if (fragments.at(ch)) {
	  outFragments += *fragments[ch];
	  const EventFragment* fragment = static_cast<const EventFragment*>(fragments[ch]->data());
	  header.status |= fragment->header.status;
	  header.fragment_count += 1;
	  header.trigger_bits |= fragment->header.trigger_bits;
	  if (!header.timestamp) { //BP: should be taken from TLB fragment if available
	    header.timestamp = fragment->header.timestamp;
	    header.bc_id = fragment->header.bc_id;
	    header.event_id = fragment->header.event_id;
	    header.event_tag = fragment->header.fragment_tag;
	  } else {
	    if (header.bc_id != fragment->header.bc_id) { //Need to handle near match for digitizer
	      WARNING("Mismatch in BCID for event "<<header.event_id<<" : "<<header.bc_id<<" != "<<fragment->header.bc_id);
	      header.status |= BCIDMismatch;
	    }
	    if (header.event_tag != fragment->header.fragment_tag) {
	      WARNING("Mismatch in event tag for event "<<header.event_id<<" : "<<header.event_tag<<" != "<<fragment->header.fragment_tag);
	      header.status |= TagMismatch;
	    }
	  }
	  delete fragments[ch];
	} else {
	  WARNING("Missing fragment for event "<<header.event_id<<": No fragment for channel "<<ch);
	  header.status |= MissingFragment;
	}
      }

      header.payload_size=outFragments.size();
      daqling::utilities::Binary data(static_cast<const void *>(&header), sizeof(header));
      data+=outFragments;
      INFO("Sending event "<<header.event_id<<" - "<<data.size()<<" bytes.");
      for ( unsigned int outch=m_numChannels+1;outch<=m_numChannels+m_numOutChannels;outch++){
          m_connections.put(outch, data);
      }
      return true;
}

void EventBuilderFaserModule::runner() {
  INFO("Running...");

  unsigned int channelNum=1;
  bool noData=true;
  std::map<uint64_t,std::vector<daqling::utilities::Binary *> > pendingFragments;
  std::map<uint64_t,unsigned int> pendingFragmentsCounts;
  std::vector<uint64_t> pendingEventIDs;
  pendingEventIDs.reserve(m_maxPending+1);
  daqling::utilities::Binary* blob = new daqling::utilities::Binary;
  while (m_run) { //BP: At the moment there is no flushing of partial events on stop
    m_queueFraction=1.0*pendingEventIDs.size()/m_maxPending;
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
    
    if (!m_connections.get(channel, *blob)) {
      if (noData && channelNum==1) std::this_thread::sleep_for(10ms);
      noData=true;
      continue;
    }
    noData=false;
    if (blob->size()<(int)sizeof(EventFragment)) {
      ERROR("Got to small fragment ("<<blob->size()<<" bytes) from channel "<<channel);
      continue;
    }
    const EventFragment* fragment=(const EventFragment*)blob->data();
    uint64_t event_id=fragment->header.event_id;
    if ((fragment->header.fragment_tag==MonitoringTag) || (fragment->header.fragment_tag==TLBMonitoringTag)) {
      INFO("Got monitoring fragment : "<<event_id<<" from channel "<<channel);
      //send monitoring fragments immediately as no other fragments are expected for thos
      std::vector<daqling::utilities::Binary *> toSend;
      toSend.push_back(blob);
      blob = new daqling::utilities::Binary;
      sendEvent(m_numChannels+1,toSend,1);
      m_monitoringEventCount+=1;
      continue;
    }
 
    INFO("Got data fragment : "<<event_id<<" from channel "<<channel << " with size " << blob->size());
    if (pendingFragments.find(event_id)==pendingFragments.end()) {
      pendingFragments[event_id]=std::vector<daqling::utilities::Binary *>(m_numChannels,0);
      pendingFragmentsCounts[event_id]=0;
      pendingEventIDs.push_back(event_id);
    }
    int ch=channel-1;
    if (pendingFragments[event_id][ch]!=0) {
      ERROR("Got same event from same source more than once!");
    } else {
      pendingFragmentsCounts[event_id]++;
    }
    pendingFragments[event_id][ch]=blob;
    blob = new daqling::utilities::Binary;

    uint64_t eventToSend=0;
    if (pendingFragmentsCounts[event_id]==m_numChannels) {
      eventToSend=event_id;
    } else if (pendingEventIDs.size()>m_maxPending) {
      WARNING("Too many events pending - will send first incomplete event");
      eventToSend=pendingEventIDs[0];
    }
    
    if (eventToSend) {
      sendEvent(m_numChannels+1,pendingFragments[eventToSend],m_numChannels);
      m_physicsEventCount+=1;
      pendingFragments.erase(pendingFragments.find(eventToSend));
      pendingFragmentsCounts.erase(pendingFragmentsCounts.find(eventToSend));
      pendingEventIDs.erase(std::find(pendingEventIDs.begin(),pendingEventIDs.end(),eventToSend));
    }
  }
  delete blob;
  INFO("Runner stopped");
}

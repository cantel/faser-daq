// enrico.gamberini@cern.ch

/// \cond
#include <chrono>
/// \endcond

#include <map>
#include <vector>
#include <algorithm>

#include "Modules/EventBuilderFaser.hpp"
#include "Modules/EventFormat.hpp"

#define __METHOD_NAME__ daqling::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daqling::utilities::className(__PRETTY_FUNCTION__)

using namespace std::chrono_literals;

extern "C" EventBuilder *create_object() { return new EventBuilder; }

extern "C" void destroy_object(EventBuilder *object) { delete object; }

EventBuilder::EventBuilder() {
  INFO(__METHOD_NAME__ << " With config: " << m_config.dump() << " getState: " << this->getState());
}

EventBuilder::~EventBuilder() { INFO(__METHOD_NAME__); }

void EventBuilder::start() {
  DAQProcess::start();
  INFO(__METHOD_NAME__ << " getState: " << getState());
}

void EventBuilder::stop() {
  DAQProcess::stop();
  INFO(__METHOD_NAME__ << " getState: " << this->getState());
}


void EventBuilder::runner() {
  INFO(__METHOD_NAME__ << " Running...");
  int numChannels=2;  //should get this from connection manager...
  unsigned maxPending=10; //should be configuration parameter

  int channelNum=1;
  bool noData=true;
  std::map<uint32_t,std::vector<daqling::utilities::Binary *> > pendingFragments;
  std::map<uint32_t,int> pendingFragmentsCounts;
  std::vector<uint32_t> pendingEventIDs;
  pendingEventIDs.reserve(maxPending+1);
  daqling::utilities::Binary* blob = new daqling::utilities::Binary;
  while (m_run) {
    int channel=channelNum;
    channelNum++;
    if (channelNum>numChannels) channelNum=1;
    
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
    int event_id=fragment->header.event_id;
    INFO("Got fragment : "<<event_id<<" from channel "<<channel);
    if (pendingFragments.find(event_id)==pendingFragments.end()) {
      pendingFragments[event_id]=std::vector<daqling::utilities::Binary *>(numChannels,0);
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

    uint32_t eventToSend=0;
    if (pendingFragmentsCounts[event_id]==numChannels) {
      eventToSend=event_id;
    } else if (pendingEventIDs.size()>maxPending) {
      WARNING("Too many events pending - will send first incomplete event");
      eventToSend=pendingEventIDs[0];
    }
    
    if (eventToSend) {
      daqling::utilities::Binary outFragments;
      EventHeader header;
      header.event_id = eventToSend;
      header.bc_id = 0xFFFF;
      header.timestamp = 0;
      header.num_fragments = pendingFragmentsCounts[eventToSend];
      header.status = 0;
      for(int ch=0;ch<numChannels;ch++) {
	if (pendingFragments[eventToSend][ch]) {
	  outFragments += *pendingFragments[eventToSend][ch];
	  const EventFragment* fragment = static_cast<const EventFragment*>(pendingFragments[eventToSend][ch]->data());
	  header.status |= fragment->header.status;
	  if (!header.timestamp) {
	    header.timestamp = fragment->header.timestamp;
	    header.bc_id = fragment->header.bc_id;
	  } else {
	    if (header.bc_id != fragment->header.bc_id) {
	      WARNING("Mismatch in BCID for event "<<eventToSend<<" : "<<header.bc_id<<" != "<<fragment->header.bc_id);
	      header.status |= BCIDMismatch;
	    }
	  }
	  delete pendingFragments[eventToSend][ch];
	} else {
	  header.status |= MissingFragment;
	}
      }
      pendingFragments.erase(pendingFragments.find(eventToSend));
      pendingFragmentsCounts.erase(pendingFragmentsCounts.find(eventToSend));
      pendingEventIDs.erase(std::find(pendingEventIDs.begin(),pendingEventIDs.end(),eventToSend));

      header.data_size=outFragments.size();
      daqling::utilities::Binary data(static_cast<const void *>(&header), sizeof(header));
      data+=outFragments;
      INFO("Sending event "<<eventToSend<<" - "<<data.size()<<" bytes - "<<pendingEventIDs.size()<<" incomplete events pending");
      m_connections.put(numChannels+1, data);
    }
  }
  delete blob;
  INFO(__METHOD_NAME__ << " Runner stopped");
}

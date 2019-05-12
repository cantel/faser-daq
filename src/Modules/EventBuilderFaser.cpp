// enrico.gamberini@cern.ch

/// \cond
#include <chrono>
/// \endcond

#include <map>
#include <vector>

#include "Modules/EventBuilderFaser.hpp"
#include "Modules/EventFormat.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

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
  int channelNum=1;
  unsigned maxPending=10;
  bool noData=true;
  std::map<uint32_t,std::vector<daq::utilities::Binary *> > pendingFragments;
  std::map<uint32_t,int> pendingFragmentsCounts;
  daq::utilities::Binary* blob = new daq::utilities::Binary;
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
      pendingFragments[event_id]=std::vector<daq::utilities::Binary *>(numChannels,0);
      pendingFragmentsCounts[event_id]=0;
    }
    int ch=channel-1;
    if (pendingFragments[event_id][ch]!=0) {
      ERROR("Got same event from same source more than once!");
    } else {
      pendingFragmentsCounts[event_id]++;
    }
    pendingFragments[event_id][ch]=blob;
    blob = new daq::utilities::Binary;

    uint32_t eventToSend=0;
    if (pendingFragmentsCounts[event_id]==numChannels) {
      eventToSend=event_id;
    } else if (pendingFragments.size()>maxPending) {
      WARNING("Too many events pending - will send first incomplete event");
      eventToSend=pendingFragments.begin()->first;
    }
    
    if (eventToSend) {
      daq::utilities::Binary outFragments;
      for(int ch=0;ch<numChannels;ch++) {
	if (pendingFragments[eventToSend][ch]) outFragments+=*pendingFragments[eventToSend][ch];
	delete pendingFragments[eventToSend][ch];
      }
      pendingFragments.erase(pendingFragments.find(eventToSend));
      pendingFragmentsCounts.erase(pendingFragmentsCounts.find(eventToSend));
      EventHeader header;
      header.event_id=eventToSend;
      header.bc_id=((const EventFragment*) outFragments.data())->header.bc_id;
      header.timestamp=((const EventFragment*) outFragments.data())->header.timestamp;
      header.num_fragments=pendingFragmentsCounts[eventToSend];
      header.data_size=outFragments.size();
      daq::utilities::Binary data(static_cast<const void *>(&header), sizeof(header));
      data+=outFragments;
      INFO("Sending event "<<eventToSend<<" - "<<data.size()<<" bytes");
      m_connections.put(3, data);
    }
  }
  delete blob;
  INFO(__METHOD_NAME__ << " Runner stopped");
}

#include "Modules/FrontEndEmulator.hpp"
#include "Modules/EventFormat.hpp"

#include <random>

extern "C" FrontEndEmulator *create_object() { return new FrontEndEmulator; }

extern "C" void destroy_object(FrontEndEmulator *object) { delete object; }

FrontEndEmulator::FrontEndEmulator() {
  INFO("FrontEndEmulator::FrontEndEmulator");

  auto cfg = m_config.getConfig();

  m_meanSize = cfg["meanSize"];
  m_rmsSize = cfg["rmsSize"];
  m_fragID = cfg["fragmentID"];  // to be made configurable
  INFO("Generating fragment with mean of "<<m_meanSize
       <<" bytes and RMS of "<<m_rmsSize<<" bytes");

  if (m_trigIn.init(cfg["triggerPort"])) {
    ERROR("Cannot bind trigger port");
    exit(1);
  }
  
  INFO("Will send data to board reader at "<<cfg["daqHost"]<<" port "<<cfg["daqPort"]);
  if (m_outHandle.init(cfg["daqHost"],cfg["daqPort"])) {
    ERROR("Cannot set up output");
    exit(2);
  }
}

FrontEndEmulator::~FrontEndEmulator() {
  INFO("FrontEndEmulator::~FrontEndEmulator");
}

void FrontEndEmulator::start() {
  m_eventCounter=0;
  DAQProcess::start();
  INFO("FrontEndEmulator::start");
}

void FrontEndEmulator::stop() {
  DAQProcess::stop();
  INFO("FrontEndEmulator::stop");
}

void FrontEndEmulator::runner() {
  INFO(" Running...");
  std::default_random_engine generator;
  std::normal_distribution<float> gaussian(m_meanSize,m_rmsSize);
  RawFragment data;
  for(int ii=0;ii<MAXFRAGSIZE;ii++) data.data[ii]=m_fragID;
  while (m_run) {
    TriggerMsg buffer;
    int num=m_trigIn.receive(&buffer,sizeof(buffer));
    if (num<0) continue;
    if (num!=sizeof(buffer)) {
      WARNING("Received "<<num<<" bytes");
      continue;
    }
    m_eventCounter++;
    INFO("Event "<<m_eventCounter<<": "<<buffer.event_id<<" BC:"<<buffer.bc_id);
    data.source_id=m_fragID;
    data.event_id=m_eventCounter;
    data.bc_id=buffer.bc_id;
    data.dataLength=std::min(std::max(int(gaussian(generator)),0),MAXFRAGSIZE);
    INFO("Fragment size: "<<data.size());
    m_outHandle.send(&data,data.size());
  }
  INFO(" Runner stopped");
}

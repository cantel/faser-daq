#include "FrontEndEmulatorModule.hpp"

#include <random>

FrontEndEmulatorModule::FrontEndEmulatorModule() {
  INFO("");

  auto cfg = m_config.getConfig()["settings"];

  m_meanSize = cfg["meanSize"];
  m_rmsSize = cfg["rmsSize"];
  m_fragID = cfg["fragmentID"];  
  INFO("Generating fragment "<<m_fragID<<" with mean of "<<m_meanSize
       <<" bytes and RMS of "<<m_rmsSize<<" bytes");
  
  m_probMissTrig = cfg["probMissingTrigger"];
  m_probMissFrag = cfg["probMissingFragment"];
  m_probCorruptFrag = cfg["probCorruptedFragment"];
		      
  m_monitoringInterval = seconds(cfg["monitoringInterval"]);

  m_monFrag.type=monType;
  m_monFrag.source_id=m_fragID;
  
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

FrontEndEmulatorModule::~FrontEndEmulatorModule() {
  INFO("");
}

void FrontEndEmulatorModule::start(int run_num) {
  m_eventCounter=0;
  DAQProcess::start(run_num);
  INFO("");
}

void FrontEndEmulatorModule::stop() {
  DAQProcess::stop();
  INFO("");
}

static microseconds timeNow() {
  return duration_cast<microseconds>(system_clock::now().time_since_epoch());
}

void FrontEndEmulatorModule::runner() {
  INFO("Running...");
  std::default_random_engine generator;
  std::normal_distribution<float> gaussian(m_meanSize,m_rmsSize);
  std::uniform_real_distribution<> flat(0.0, 1.0);
  RawFragment data;
  for(int ii=0;ii<MAXFRAGSIZE;ii++) data.data[ii]=m_fragID;
  m_monFrag.counter = 0;
  m_monFrag.num_fragments_sent = 0;
  m_monFrag.size_fragments_sent = 0;
  m_timeMonitoring = timeNow();
  while (m_run) {
    TriggerMsg buffer;
    int num=m_trigIn.receive(&buffer,sizeof(buffer));
    if (num<0) continue;
    if (num!=sizeof(buffer)) {
      WARNING("Received "<<num<<" bytes");
      continue;
    }
    if (flat(generator)<m_probMissTrig) {
      INFO("Emulating missed trigger, global id:"<<buffer.event_id<<" - local counter: "<<m_eventCounter);
      continue;
    }
    m_eventCounter++;
    INFO("Event "<<m_eventCounter<<": "<<buffer.event_id<<" BC:"<<buffer.bc_id);
    data.type       = dataType;
    data.source_id  = m_fragID;
    data.event_id   = m_eventCounter;
    data.bc_id      = buffer.bc_id;
    data.dataLength = std::min(std::max(int(gaussian(generator)),0),MAXFRAGSIZE);
    INFO("Fragment size: "<<data.sizeBytes()<<" bytes");
    if (flat(generator)<m_probMissFrag) {
      INFO("Emulating missed fragment for global id:"<<buffer.event_id);
      continue;
    }
    int numBytes=data.sizeBytes();
    if (flat(generator)<m_probCorruptFrag) {
      data.type=uint32_t(flat(generator)*0xFFFFFFFF);
      data.event_id=uint32_t(flat(generator)*0xFFFFFFFF);
      data.source_id=uint32_t(flat(generator)*0xFFFFFFFF);
      data.bc_id=uint16_t(flat(generator)*0xFFFF);
      data.dataLength=uint16_t(flat(generator)*0xFFFF);
      numBytes=std::min(int(flat(generator)*0xFFFF),MAXFRAGSIZE);
    }
    m_outHandle.send(&data,numBytes);
    m_monFrag.num_fragments_sent+=1;
    m_monFrag.size_fragments_sent+=numBytes;
    if (m_monitoringInterval!=microseconds::zero() &&
	((timeNow()-m_timeMonitoring)>m_monitoringInterval)) {
      m_outHandle.send(&m_monFrag,sizeof(m_monFrag));
      INFO("Send monitoring fragment "<<m_monFrag.counter<<": "<<
	   m_monFrag.num_fragments_sent<<" fragments with "<<m_monFrag.size_fragments_sent<<" bytes");
      m_monFrag.counter++;
      m_monFrag.num_fragments_sent=0;
      m_monFrag.size_fragments_sent=0;
      m_timeMonitoring = timeNow();
    }

  }
  INFO(" Runner stopped");
}

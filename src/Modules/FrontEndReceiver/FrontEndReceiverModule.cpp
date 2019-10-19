
/// \cond
#include <iomanip>
/// \endcond

#include "FrontEndReceiverModule.hpp"

#include "Commons/EventFormat.hpp"
#include "Commons/RawExampleFormat.hpp"  

FrontEndReceiverModule::FrontEndReceiverModule() {
  INFO("With config: " << m_config.dump());

  auto cfg = m_config.getSettings();

  if (m_dataIn.init(cfg.value("dataPort",0))) {
    ERROR("Cannot bind data port");
    exit(1);
  }

}

FrontEndReceiverModule::~FrontEndReceiverModule() { }

void FrontEndReceiverModule::start(int run_num) {
  DAQProcess::start(run_num);
  INFO("getState: " << this->getState());
  m_recvCount = 0;
  if (m_stats_on) {
    m_statistics->registerVariable<std::atomic<int>, int>(&m_recvCount, "RecvCount", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);
  }
}

void FrontEndReceiverModule::stop() {
  DAQProcess::stop();
  INFO("getState: " << this->getState());
}

void FrontEndReceiverModule::runner() {
  INFO("Running...");
  while (m_run) {
    RawFragment buffer;
    int payload_size = m_dataIn.receive(&buffer,sizeof(buffer));
    m_recvCount+=1;
    if (payload_size < 0) continue;
    if (payload_size < (int) sizeof(uint32_t)*buffer.headerwords()) {
      WARNING("Received only"<<payload_size<<" bytes");
      continue;
    }
    uint8_t fragment_tag = PhysicsTag;
    uint64_t event_id=0;   // should add ECR counter her
    uint16_t bc_id=0;
    uint32_t source_id=0;  //most likely the system source should be set here and sub-board from payload (or config as well)
    uint16_t status=0;
    if (buffer.type!=monType) {
      event_id  = buffer.event_id;  
      source_id = buffer.source_id;
      bc_id     = buffer.bc_id;
      if (payload_size != buffer.sizeBytes()) {
	status |= EventStatus::CorruptedFragment;
	WARNING("Got corrupted event, event id "<<event_id<<" - "<<payload_size<<" != "<<buffer.sizeBytes());
      } 
    } else {
      MonitoringFragment* monData=(MonitoringFragment*)&buffer;
      fragment_tag = MonitoringTag;
      event_id = monData->counter;
      source_id = monData->source_id;
      bc_id = 0xFFFF;
    }
    Binary rawData(&buffer,payload_size);
    std::unique_ptr<EventFragment> fragment(new EventFragment(fragment_tag, source_id, event_id, bc_id, rawData));
    fragment->set_status(status);

    m_connections.put(1, const_cast<Binary&>(fragment->raw())); //BP: put() is not declared const...

  }

  INFO(" Runner stopped");
}

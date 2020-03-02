
/// \cond
#include <iomanip>
/// \endcond

#include "FrontEndReceiverModule.hpp"

#include "EventFormats/DAQFormats.hpp"
#include "EventFormats/RawExampleFormat.hpp"  
#include <Utils/Binary.hpp>

using namespace DAQFormats;

FrontEndReceiverModule::FrontEndReceiverModule() {
  INFO("With config: " << m_config.dump());

  auto cfg = m_config.getSettings();

  if (m_dataIn.init(cfg.value("dataPort",0))) {
    ERROR("Cannot bind data port");
    exit(1);
  }

}

FrontEndReceiverModule::~FrontEndReceiverModule() { }

void FrontEndReceiverModule::configure() {
  FaserProcess::configure();
  registerVariable(m_recvCount,"RecvCount");  // variable is reset to zero here, but any reset on start of run has to be added to start() manually
  
  // any onetime electronics configuration should be done here
  // in case of issues, set m_status to STATUS_WARN or STATUS_ERROR
  // For fatal error set m_state="failed" as well to prevent starting run
}

void FrontEndReceiverModule::sendECR() {
  // should send ECR to electronics here. In case of failure, seet m_status to STATUS_ERROR
}

void FrontEndReceiverModule::start(unsigned int run_num) {
  FaserProcess::start(run_num);
  INFO("getState: " << this->getState());
}

void FrontEndReceiverModule::stop() {
  FaserProcess::stop();
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
    uint64_t event_id=0;   
    uint16_t bc_id=0;
    uint32_t source_id=0;  //most likely the system source should be set here from and sub-board from payload (or from config)
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
    event_id|=m_ECRcount<<24;
    std::unique_ptr<EventFragment> fragment(new EventFragment(fragment_tag, source_id, event_id, bc_id, &buffer,payload_size));
    fragment->set_status(status);
    std::unique_ptr<const byteVector> bytestream(fragment->raw());
    daqling::utilities::Binary binData(bytestream->data(),bytestream->size());

    m_connections.put(0,binData);

  }

  INFO(" Runner stopped");
}

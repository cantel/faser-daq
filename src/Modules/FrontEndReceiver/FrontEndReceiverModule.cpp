
/// \cond
#include <chrono>
#include <iomanip>
/// \endcond

#include "FrontEndReceiverModule.hpp"

#include "Commons/EventFormat.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;

FrontEndReceiverModule::FrontEndReceiverModule() {
  INFO("With config: " << m_config.dump());

  auto cfg = m_config.getConfig()["settings"];

  if (m_dataIn.init(cfg["dataPort"])) {
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
  const unsigned source_id = 1;
  unsigned sequence_number = 0;
  microseconds timestamp;

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
    timestamp = duration_cast<microseconds>(system_clock::now().time_since_epoch());
    const int total_size = sizeof(EventFragmentHeader) + sizeof(char) * payload_size;

    INFO("sequence number " << sequence_number << "  >>  timestamp " << std::hex
                         << "0x" << timestamp.count() << std::dec << "  >>  payload size "
                         << payload_size);

    std::unique_ptr<EventFragment> data(new EventFragment);
    data->header.marker = FragmentMarker;
    data->header.fragment_tag = PhysicsTag;
    data->header.trigger_bits = 0;
    data->header.version_number = EventFragmentVersion;
    data->header.header_size = sizeof(data->header);
    data->header.payload_size = payload_size;
    data->header.status = 0;
    if (buffer.type!=monType) {
      data->header.event_id = buffer.event_id;  
      data->header.source_id = buffer.source_id;
      data->header.bc_id = buffer.bc_id;
      if (payload_size != buffer.sizeBytes()) {
	data->header.status |= CorruptedFragment;
	WARNING("Got corrupted event, event id "<<data->header.event_id<<" - "<<payload_size<<" != "<<buffer.sizeBytes());
      }
    } else {
      MonitoringFragment* monData=(MonitoringFragment*)&buffer;
      data->header.event_id = monData->counter;
      data->header.source_id = monData->source_id;
      data->header.bc_id = 0xFFFF;
      data->header.fragment_tag = MonitoringTag;
    }
    data->header.timestamp = timestamp.count();
    memcpy(data->payload, &buffer, payload_size);

    // ready to be sent to EB
    auto binary = daqling::utilities::Binary(static_cast<const void *>(data.get()), total_size);

    // print binary
    if (sequence_number%10==0) 
      std::cout << binary << std::endl;

    m_connections.put(1, binary);

    sequence_number++;
  }
  INFO(" Runner stopped");
}

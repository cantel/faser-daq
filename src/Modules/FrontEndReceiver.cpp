
/// \cond
#include <chrono>
#include <iomanip>
/// \endcond

#include "Modules/FrontEndReceiver.hpp"

#include "Modules/EventFormat.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace std::chrono_literals;
using namespace std::chrono;

extern "C" FrontEndReceiver *create_object(std::string name, int num) {
  return new FrontEndReceiver(name, num);
}

extern "C" void destroy_object(FrontEndReceiver *object) { delete object; }

FrontEndReceiver::FrontEndReceiver(std::string name, int num) {
  INFO(__METHOD_NAME__ << " Passed " << name << " " << num << " with constructor");
  INFO(__METHOD_NAME__ << " With config: " << m_config.dump());

  auto cfg = m_config.getConfig();

  if (m_dataIn.init(cfg["dataPort"])) {
    ERROR("Cannot bind data port");
    exit(1);
  }
  
}

FrontEndReceiver::~FrontEndReceiver() { INFO(__METHOD_NAME__); }

void FrontEndReceiver::start() {
  DAQProcess::start();
  INFO(__METHOD_NAME__ << " getState: " << this->getState());
}

void FrontEndReceiver::stop() {
  DAQProcess::stop();
  INFO(__METHOD_NAME__ << " getState: " << this->getState());
}

void FrontEndReceiver::runner() {
  const unsigned source_id = 1;
  unsigned sequence_number = 0;
  microseconds timestamp;

  INFO(__METHOD_NAME__ << " Running...");
  while (m_run) {
    RawFragment buffer;
    int payload_size = m_dataIn.receive(&buffer,sizeof(buffer));
    if (payload_size < 0) continue;
    if (payload_size < 4*buffer.headersize()) {
      WARNING("Received only"<<payload_size<<" bytes");
      continue;
    }
    timestamp = duration_cast<microseconds>(system_clock::now().time_since_epoch());
    const int total_size = sizeof(EventFragmentHeader) + sizeof(char) * payload_size;

    INFO(__METHOD_NAME__ << " sequence number " << sequence_number << "  >>  timestamp " << std::hex
                         << "0x" << timestamp.count() << std::dec << "  >>  payload size "
                         << payload_size);

    std::unique_ptr<EventFragment> data((EventFragment *)malloc(total_size));
    data->header.event_id = buffer.event_id;  //Could add sequence number for received fragment, but merging must be done by frontend event_id, so that is main number. Also error flag is missing
    data->header.source_id = buffer.source_id;
    data->header.bc_id = buffer.bc_id;
    data->header.payload_size = payload_size;

    data->header.timestamp = timestamp.count();
    memcpy(data->payload, &buffer, payload_size);

    // ready to be sent to EB
    auto binary = daq::utilities::Binary(static_cast<const void *>(data.get()), total_size);

    // print binary
    std::cout << binary << std::endl;

    m_connections.put(1, binary);

    sequence_number++;
  }
  INFO(__METHOD_NAME__ << " Runner stopped");
}

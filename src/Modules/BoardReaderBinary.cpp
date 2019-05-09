// enrico.gamberini@cern.ch

/// \cond
#include <chrono>
#include <iomanip>
/// \endcond

#include "Modules/BoardReader.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace std::chrono_literals;
using namespace std::chrono;

struct header_t {
  uint16_t payload_size;
  uint16_t source_id;
  uint32_t seq_number;
  uint64_t timestamp;
} __attribute__((__packed__));

struct data_t {
  header_t header;
  char payload[];
} __attribute__((__packed__));

extern "C" BoardReader *create_object(std::string name, int num) {
  return new BoardReader(name, num);
}

extern "C" void destroy_object(BoardReader *object) { delete object; }

BoardReader::BoardReader(std::string name, int num) {
  INFO(__METHOD_NAME__ << " Passed " << name << " " << num << " with constructor");
  INFO(__METHOD_NAME__ << " With config: " << m_config.dump());
}

BoardReader::~BoardReader() { INFO(__METHOD_NAME__); }

void BoardReader::start() {
  DAQProcess::start();
  INFO(__METHOD_NAME__ << " getState: " << this->getState());
}

void BoardReader::stop() {
  DAQProcess::stop();
  INFO(__METHOD_NAME__ << " getState: " << this->getState());
}

void BoardReader::runner() {
  const unsigned source_id = 1;
  unsigned sequence_number = 0;
  microseconds timestamp;

  INFO(__METHOD_NAME__ << " Running...");
  while (m_run) {
    timestamp = duration_cast<microseconds>(system_clock::now().time_since_epoch());
    const unsigned payload_size = (rand() % 128 + 1);
    const unsigned total_size = sizeof(data_t) + sizeof(char) * payload_size;

    INFO(__METHOD_NAME__ << " sequence number " << sequence_number << "  >>  timestamp " << std::hex
                         << "0x" << timestamp.count() << std::dec << "  >>  payload size "
                         << payload_size);

    std::unique_ptr<data_t> data((data_t *)malloc(total_size));
    data->header.payload_size = payload_size;
    data->header.seq_number = sequence_number;
    data->header.source_id = source_id;
    data->header.timestamp = timestamp.count();
    memset(data->payload, 0xFE, payload_size);

    // ready to be sent to EB
    auto binary = daq::utilities::Binary(static_cast<const void *>(data.get()), total_size);

    // print binary
    std::cout << binary << std::endl;

    m_connections.put(1, binary);

    sequence_number++;
    std::this_thread::sleep_for(500ms);
  }
  INFO(__METHOD_NAME__ << " Runner stopped");
}

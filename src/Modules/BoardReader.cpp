// enrico.gamberini@cern.ch

/// \cond
#include <chrono>
/// \endcond

#include "Modules/BoardReader.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace std::chrono_literals;
using namespace std::chrono;

struct header_t {
  uint32_t payload_size;
  uint32_t source_id;
  uint32_t seq_number;
  uint64_t timestamp;
};

struct data_t {
  header_t header;
  char payload[];
};

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
  // const unsigned source_id = 1;
  // unsigned sequence_number = 0;
  // microseconds timestamp;

  INFO(__METHOD_NAME__ << " Running...");
  while (m_run) {
    m_connections.putStr(1, "WoofBla");

    // timestamp = duration_cast<microseconds>(system_clock::now().time_since_epoch());
    // const unsigned payload_size = rand() % 1000 + 1;
    // INFO(__METHOD_NAME__ << " sequence number " << sequence_number
    //     << "  >>  timestamp " << timestamp.count() << "  >>  payload size " << payload_size );

    // std::unique_ptr<data_t> data((data_t*)malloc(sizeof(data_t) + sizeof(char) * payload_size));
    // data->header.payload_size = payload_size;
    // data->header.seq_number = sequence_number;
    // data->header.source_id = source_id;
    // data->header.timestamp = timestamp.count();
    // memset(data->payload, 'F', payload_size);
    // ready to be sent to EB

    // sequence_number++;
    std::this_thread::sleep_for(10ms);
  }
  INFO(__METHOD_NAME__ << " Runner stopped");
}

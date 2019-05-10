/// \cond
#include <chrono>
/// \endcond
using namespace std::chrono_literals;

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

#include "Modules/Monitor.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

extern "C" Monitor *create_object() { return new Monitor; }

extern "C" void destroy_object(Monitor *object) { delete object; }

Monitor::Monitor() { INFO("Monitor::Monitor"); }

Monitor::~Monitor() { 
  //INFO(__METHOD_NAME__ << " With config: " << m_config.dump() << " getState: " << this->getState());
  INFO(__METHOD_NAME__ << " getState: " << this->getState());
 }

void Monitor::start() {
  DAQProcess::start();
  INFO(__METHOD_NAME__ << " getState: " << this->getState());
}

void Monitor::stop() {
  DAQProcess::stop();
  INFO(__METHOD_NAME__ << " getState: " << this->getState());
}

void Monitor::runner() {
  INFO(__METHOD_NAME__ << " Running...");
  while (m_run) {
    daq::utilities::Binary b1;
    while(!m_connections.get(1, b1) && m_run) {
      INFO("b1.size() =" << b1.size());
      const data_t * unpackeddata((data_t *)malloc(b1.size()));
      //unpackeddata = static_cast<const data_t *>(b1.data());
      //INFO(__METHOD_NAME__ <<  " data sequence number "<< unpackeddata->header.seq_number);
      std::this_thread::sleep_for(100ms);
    }
  }
  INFO(__METHOD_NAME__ << " Runner stopped");
}

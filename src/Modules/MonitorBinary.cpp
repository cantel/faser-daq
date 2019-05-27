/// \cond
#include <chrono>
/// \endcond
using namespace std::chrono_literals;


#include <boost/format.hpp>
#include <boost/histogram.hpp>
using namespace boost::histogram;
#include <iostream> // std::flush
#include <sstream> // std::ostringstream

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

#define __METHOD_NAME__ daqling::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daqling::utilities::className(__PRETTY_FUNCTION__)

extern "C" Monitor *create_object() { return new Monitor; }

extern "C" void destroy_object(Monitor *object) { delete object; }

Monitor::Monitor() { INFO("Monitor::Monitor"); }

Monitor::~Monitor() { 
  INFO(__METHOD_NAME__ << " With config: " << m_config.dump() << " getState: " << this->getState());
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

  auto testhist = make_histogram(axis::regular<>(150, -0.5, 149.5, "x"));
 
  while (m_run) {
    INFO(__METHOD_NAME__ << "just checked. still running ..");
    daqling::utilities::Binary b1;
    while(!m_connections.get(1, b1) && m_run) {
      std::this_thread::sleep_for(100ms);
    }
    //while ( m_connections.get(1, b1) && m_run ) {
    INFO("b1.size() =" << b1.size());
    if ( b1.size() > 0 ) {
        const data_t * unpackeddata((data_t *)malloc(b1.size()));
    	unpackeddata = static_cast<const data_t *>(b1.data());
    	INFO(__METHOD_NAME__ <<  " data sequence number "<< unpackeddata->header.seq_number);
    	INFO(__METHOD_NAME__ <<  " data payload size "<< unpackeddata->header.payload_size);
    	testhist(unpackeddata->header.payload_size);
    }
  }

  INFO(__METHOD_NAME__ << " flushing testhist info ... ");

  // stolen from c++ site.
  std::ostringstream os;
  for (auto x : indexed(testhist, coverage::all)) {
    os << boost::format("bin %2i [%4.1f, %4.1f): %i\n") % x.index() % x.bin().lower() %
              x.bin().upper() % *x;
  }

  std::cout << os.str() << std::flush;

  INFO(__METHOD_NAME__ << " Runner stopped");
}

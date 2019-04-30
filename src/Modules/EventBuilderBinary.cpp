// enrico.gamberini@cern.ch

/// \cond
#include <chrono>
/// \endcond

#include "Modules/EventBuilder.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace std::chrono_literals;

extern "C" EventBuilder *create_object() { return new EventBuilder; }

extern "C" void destroy_object(EventBuilder *object) { delete object; }

EventBuilder::EventBuilder() {
  INFO(__METHOD_NAME__ << " With config: " << m_config.dump() << " getState: " << this->getState());
}

EventBuilder::~EventBuilder() { INFO(__METHOD_NAME__); }

void EventBuilder::start() {
  DAQProcess::start();
  INFO(__METHOD_NAME__ << " getState: " << getState());
}

void EventBuilder::stop() {
  DAQProcess::stop();
  INFO(__METHOD_NAME__ << " getState: " << this->getState());
}

void EventBuilder::runner() {
  INFO(__METHOD_NAME__ << " Running...");
  while (m_run) {
    daq::utilities::Binary b1, b2;
    while(!m_connections.get(1, b1) && m_run) {
      std::this_thread::sleep_for(100ms);
    }
    while(!m_connections.get(2, b2) && m_run) {
      std::this_thread::sleep_for(100ms);
    }

    daq::utilities::Binary b3(b1);
    b3 += b2;
    m_connections.put(3, b3);
  }
  INFO(__METHOD_NAME__ << " Runner stopped");
}

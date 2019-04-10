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
  INFO(__METHOD_NAME__ << " getState: " << getState());

  m_runner_thread = std::make_unique<std::thread>(&EventBuilder::runner, this);
}

void EventBuilder::stop() {
  INFO(__METHOD_NAME__ << " getState: " << this->getState());

  m_runner_thread->join();
}

void EventBuilder::runner() {
  auto &cm = daq::core::ConnectionManager::instance();

  INFO(__METHOD_NAME__ << " Running...");
  const unsigned c_packing = 20;
  while (m_run) {
    std::string packed = "";
    for (unsigned i = 0; i < c_packing;) {
      std::string s1{cm.getStr(1)};
      std::string s2{cm.getStr(2)};
      if (s1 != "") {
        INFO("Received on channel 1 " << s1);
        packed += s1;
        i++;
      }
      if (s2 != "") {
        INFO("Received on channel 2 " << s2);
        packed += s2;
        i++;
      }
    }
    // if (packed != "") {
    //   INFO("Sending mega string " << packed);
    //   cm.putStr(3, packed);
    // }
  }
  INFO(__METHOD_NAME__ << " Runner stopped");
}

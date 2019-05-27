#include "Modules/Monitor.hpp"

#define __METHOD_NAME__ daqling::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daqling::utilities::className(__PRETTY_FUNCTION__)

extern "C" Monitor *create_object() { return new Monitor; }

extern "C" void destroy_object(Monitor *object) { delete object; }

Monitor::Monitor() { INFO("Monitor::Monitor"); }

Monitor::~Monitor() { INFO("Monitor::~Monitor"); }

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
  const unsigned c_packing = 20;
  while (m_run) {
    //std::string packed = "";
    //for (unsigned i = 0; i < c_packing;) {
      std::string s1{m_connections.getStr(1)};
      if (s1 != "") {
        INFO("Received on channel 1 " << s1);
        //packed += s1;
        //i++;
      }
    //}
  }
  INFO(__METHOD_NAME__ << " Runner stopped");
}

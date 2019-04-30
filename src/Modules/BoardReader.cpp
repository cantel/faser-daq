// enrico.gamberini@cern.ch

/// \cond
#include <chrono>
/// \endcond

#include "Modules/BoardReader.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace std::chrono_literals;
using namespace std::chrono;

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
  INFO(__METHOD_NAME__ << " Running...");
  while (m_run) {
    m_connections.putStr(1, "WoofBla");

    std::this_thread::sleep_for(10ms);
  }
  INFO(__METHOD_NAME__ << " Runner stopped");
}

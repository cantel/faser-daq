#include "Modules/TriggerGenerator.hpp"
#include "Modules/EventFormat.hpp"

#include <random>

extern "C" TriggerGenerator *create_object() { return new TriggerGenerator; }

extern "C" void destroy_object(TriggerGenerator *object) { delete object; }

TriggerGenerator::TriggerGenerator() {
  INFO("TriggerGenerator::TriggerGenerator");
  auto cfg = m_config.getConfig();
  m_rate = cfg["rateInHz"];
  INFO("Triggers generated at "<<m_rate<<" Hz");
  auto dests = cfg["frontendApps"];
  if (dests.empty()) {
    ERROR("No frontend applications specified");
    cfg.dump();
  }
  for ( auto& dest : dests) {
    INFO(" Frontend at host: "<<dest["host"]<<", port: "<<dest["port"]);
    UdpSender *target=new UdpSender;
    if (target->init(dest["host"],dest["port"])) {
      ERROR("Failed to setup socket");
      delete target;
    } else m_targets.push_back(target);
  }
  INFO("Done");
}

TriggerGenerator::~TriggerGenerator() {
  INFO("TriggerGenerator::~TriggerGenerator");
}

void TriggerGenerator::start() {
  m_eventCounter=0;
  DAQProcess::start();
  INFO("TriggerGenerator::start");
}

void TriggerGenerator::stop() {
  DAQProcess::stop();
  INFO("TriggerGenerator::stop");
}

void TriggerGenerator::runner() {
  INFO(" Running...");
  int msgFreq=int(m_rate);
  float sleepTime=1000000./m_rate;
  std::default_random_engine generator;
  std::exponential_distribution<float> distribution(1./sleepTime);
  std::uniform_int_distribution<int> bcGen(1,3564);
  while (m_run) {
    m_eventCounter++;
    int museconds=distribution(generator);
    if (m_eventCounter % msgFreq == 0)
      INFO("Sending trigger for event: "<<m_eventCounter<<" then sleep "<<museconds<<" microseconds");
    TriggerMsg trigger;
    trigger.event_id=m_eventCounter;
    trigger.bc_id=bcGen(generator);
    
    for(auto& target : m_targets) {
      target->send(&trigger, sizeof(trigger));
    }
    usleep(museconds);
  }
  INFO(" Runner stopped");
}

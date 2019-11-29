#include "TriggerGeneratorModule.hpp"
#include "Commons/RawExampleFormat.hpp"

#include <random>

TriggerGeneratorModule::TriggerGeneratorModule() {
  INFO("");
  auto cfg = m_config.getConfig()["settings"];
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

TriggerGeneratorModule::~TriggerGeneratorModule() {
  INFO("");
}

void TriggerGeneratorModule::start(unsigned int run_num) {
  m_eventCounter=0;
  m_enabled=true;
  FaserProcess::start(run_num);
  INFO("");
}

void TriggerGeneratorModule::stop() {
  FaserProcess::stop();
  INFO("");
}

void TriggerGeneratorModule::enableTrigger(const std::string &/*arg*/) {
  m_enabled=true;
  INFO("Enabled trigger");
}

void TriggerGeneratorModule::disableTrigger(const std::string &/*arg*/) {
  m_enabled=false;
  INFO("Disabled trigger");
}

void TriggerGeneratorModule::runner() {
  INFO(" Running...");
  int msgFreq=int(m_rate);
  float sleepTime=1000000./m_rate;
  std::default_random_engine generator;
  std::exponential_distribution<float> distribution(1./sleepTime);
  std::uniform_int_distribution<int> bcGen(1,3564);
  while (m_run) {
    int museconds=distribution(generator);

    if (m_enabled) {
      m_eventCounter++;
      TriggerMsg trigger;
      trigger.event_id=m_eventCounter;
      trigger.bc_id=bcGen(generator);
      if (m_eventCounter % msgFreq == 0)
	INFO("Sending trigger for event: "<<m_eventCounter<<" then sleep "<<museconds<<" microseconds");
      for(auto& target : m_targets) {
	target->send(&trigger, sizeof(trigger));
      }
    }
    usleep(museconds);
  }
  INFO(" Runner stopped");
}

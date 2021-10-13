/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#include "TriggerGeneratorModule.hpp"
#include "EventFormats/RawExampleFormat.hpp"

#include <random>

TriggerGeneratorModule::TriggerGeneratorModule(const std::string& n):FaserProcess(n) {
  INFO("");
  auto cfg = getModuleSettings();
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
  registerCommand("setRate", "rateSetting", "running", &TriggerGeneratorModule::setRate, this, _1);
  INFO("Done");
}

TriggerGeneratorModule::~TriggerGeneratorModule() {
  INFO("");
}

void TriggerGeneratorModule::configure() {
  FaserProcess::configure();
  registerVariable(m_triggerCount,"Triggers");
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

void TriggerGeneratorModule::setRate(const std::string &arg) {
  INFO("Set rate: "<<arg);
  m_newrate=std::stoi(arg);
}

void TriggerGeneratorModule::runner() noexcept {
  INFO(" Running...");
  int msgFreq=int(m_rate);
  double sleepTime=1000000./m_rate;
  std::default_random_engine generator;
  std::exponential_distribution<double> distribution(1./sleepTime);
  std::uniform_int_distribution<int> bcGen(1,3564);
  while (m_run) {
    if (m_newrate>0) {
      INFO("New rate: "<<m_newrate);
      msgFreq=int(m_newrate);
      std::exponential_distribution<double>::param_type newlambda(m_newrate/1000000.);
      distribution.param(newlambda);
      m_newrate=0;
    }
    int museconds=distribution(generator);

    if (m_enabled) {
      m_eventCounter++;
      TriggerMsg trigger;
      trigger.event_id=m_eventCounter;
      trigger.bc_id=bcGen(generator);
      m_triggerCount++;
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

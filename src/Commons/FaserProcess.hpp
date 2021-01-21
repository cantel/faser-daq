/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include "Core/DAQProcess.hpp"

using namespace daqling::core;

class FaserProcess: public daqling::core::DAQProcess {
public:
  enum StatusFlags { STATUS_OK=0,STATUS_WARN,STATUS_ERROR };

  FaserProcess() {  INFO("Booting with config: " << m_config.getConfig().dump(4)); }

  virtual ~FaserProcess() {}

  virtual void configure() {
    DAQProcess::configure();
    registerVariable(m_status,"Status");

    registerCommand("ECR", "sendingECR","paused",&FaserProcess::ECRcommand,this,_1);
    registerCommand("enableTrigger","enablingTrigger", "running",&FaserProcess::enableTrigger,this,_1);
    registerCommand("disableTrigger", "pausingTrigger", "paused",&FaserProcess::disableTrigger,this,_1);
  }

  void ECRcommand(const std::string &arg) {
    INFO("Got ECR command with argument "<<arg);
    m_ECRcount+=1;
    sendECR();
  }

  virtual void enableTrigger(const std::string &arg) {
    INFO("Got enableTrigger command with argument "<<arg);
    // everything but the TLB process should ignore this
  }

  virtual void disableTrigger(const std::string &arg) {
    INFO("Got disableTrigger command with argument "<<arg);
    // everything but the TLB proces should ignore this
  }

  virtual void sendECR() {}; //to be overloaded for frontend applications

  virtual void start(unsigned int run_num) {
    m_ECRcount=0;
    DAQProcess::start(run_num);
  }
  
  virtual void stop() {
    DAQProcess::stop();
  }

protected:
  //simple metrics interface. Note variables are zero'd
  void registerVariable(std::atomic<int> &var,std::string name,metrics::metric_type mtype=metrics::LAST_VALUE, float delta_t = 1) {
    var=0;
    if (m_stats_on) {
      m_statistics->registerMetric<std::atomic<int>>(&var, name, mtype, delta_t);
    }
  }
  void registerVariable(std::atomic<float> &var,std::string name,metrics::metric_type mtype=metrics::LAST_VALUE, float delta_t = 1) {
    var=0;
    if (m_stats_on) {
      m_statistics->registerMetric<std::atomic<float>>(&var, name, mtype,delta_t);
    }
  }

protected:
  std::atomic<int> m_ECRcount;
  std::atomic<int> m_status;
};
  

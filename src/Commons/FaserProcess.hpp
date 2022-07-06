/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include <string>
#include <regex>
#include <fstream>
#include <vector>

#include "Core/DAQProcess.hpp"

using namespace daqling::core;
using json = nlohmann::json;

class FaserProcess: public daqling::core::DAQProcess {
public:
  enum StatusFlags { STATUS_OK=0,STATUS_WARN,STATUS_ERROR };

  FaserProcess(const std::string& n):daqling::core::DAQProcess(n) {  INFO("Booting with config: " << m_config.getConfig().dump(4)); }

  virtual ~FaserProcess() {}

  virtual void configure() {
    //    DAQProcess::configure(); // replaced with local instance to allow environment variabls in influxdb string
    std::string influxDbURI = m_config.getMetricsSettings()["influxDb_uri"];
    std::ifstream secretsFile("/etc/faser-secrets.json");
    if (secretsFile.good()) {
      json secrets;
      secretsFile >> secrets;
      INFO("Original influx path: "+influxDbURI);
      autoExpandEnvironmentVariables(influxDbURI,secrets);
      INFO("After variable replacement: "+influxDbURI);
    }
    m_config.getMetricsSettings()["influxDb_uri"] = influxDbURI;
    DAQProcess::configure();

    registerVariable(m_status,"Status");

    registerCommand("ECR", "sendingECR","paused",&FaserProcess::ECRcommand,this,_1);
    registerCommand("enableTrigger","enablingTrigger", "running",&FaserProcess::enableTrigger,this,_1);
    registerCommand("disableTrigger", "pausingTrigger", "paused",&FaserProcess::disableTrigger,this,_1);
  }

  void autoExpandEnvironmentVariables( std::string & text, json & vars ) {
    static std::regex env( "\\$\\{([^}]+)\\}" );
    std::smatch match;
    while ( std::regex_search( text, match, env ) ) {
        std::string var = "";
	if (vars.contains(match[1].str())) var=vars[match[1].str()];
        text.replace( match[0].first, match[0].second, var );
    }
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
    for(auto var : m_metric_ints) {
      *var=0;
    }
    for(auto var : m_metric_uints) {
      *var=0;
    }
    for(auto var : m_metric_floats) {
      *var=0;
      }
    DAQProcess::start(run_num);
  }
  
  virtual void stop() {
    DAQProcess::stop();
  }

protected:
  //simple metrics interface. Note variables are zero'd
  void registerVariable(std::atomic<int> &var,std::string name,metrics::metric_type mtype=metrics::LAST_VALUE, bool zero_on_start=true) {
    if (zero_on_start) {
      var=0;
      m_metric_ints.push_back(&var);
    }
    if (m_statistics->isStatsOn()) {
      m_statistics->registerMetric<std::atomic<int>>(&var, name, mtype);
    }
  }
  void registerVariable(std::atomic<size_t> &var,std::string name,metrics::metric_type mtype=metrics::LAST_VALUE, bool zero_on_start=true) {
    if (zero_on_start) {
      var=0;
      m_metric_uints.push_back(&var);
    }
    if (m_statistics->isStatsOn()) {
      m_statistics->registerMetric<std::atomic<size_t>>(&var, name, mtype);
    }
  }
  void registerVariable(std::atomic<float> &var,std::string name,metrics::metric_type mtype=metrics::LAST_VALUE, bool zero_on_start=true) {
    if (zero_on_start) {
      var=0;
      m_metric_floats.push_back(&var);
    }
    if (m_statistics->isStatsOn()) {
      m_statistics->registerMetric<std::atomic<float>>(&var, name, mtype);
    }
  }

protected:
  std::atomic<int> m_ECRcount;
  std::atomic<int> m_status;
  std::vector<std::atomic<int>*> m_metric_ints;
  std::vector<std::atomic<size_t>*> m_metric_uints;
  std::vector<std::atomic<float>*> m_metric_floats;
};
  

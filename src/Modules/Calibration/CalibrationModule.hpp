/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#pragma once

#include "Commons/FaserProcess.hpp"

class CalibrationModule : public FaserProcess {
 public:
  CalibrationModule(const std::string& n);
  ~CalibrationModule();

  void configure(); // optional (configuration can be handled in the constructor)
  void start(unsigned);
  void stop();

  void runner() noexcept;

 private:
  std::string m_tcalibLocation;
  char m_tcalibCommand[500];
  std::string m_configLocation;
  std::vector<int> m_testList;
  std::string m_outBaseDir{"."};
  int m_verboseLevel{0};
  int m_l1delay{130};
  bool m_noRunNumber{false};
  bool m_usb{false};
  
};

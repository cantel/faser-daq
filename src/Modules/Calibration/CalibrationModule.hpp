/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#pragma once

#include "Commons/FaserProcess.hpp"
#include "TrackerReadout/TRBAccess.h"

class CalibrationModule : public FaserProcess {
 public:
  CalibrationModule(const std::string& n);
  ~CalibrationModule();

  void configure(); // optional (configuration can be handled in the constructor)
  void start(unsigned);
  void stop();

  void runner() noexcept;

 private:
  // tcalib parameters
  std::string m_configLocation;
  std::vector<int> m_testList;
  std::string m_outBaseDir{"."};
  int m_verboseLevel{0};
  int m_l1delay{130};
  std::string m_log{"log.txt"};
  bool m_emulateTRB{false};
  bool m_calLoop{false};
  bool m_saveDaq{false};
  bool m_noTrim{false};
  bool m_noRunNumber{false};
  bool m_usb{false};
  std::string m_ip{"10.11.65.8"};

  // calibmanager 
  FASER::TRBAccess *m_trb;
  
};

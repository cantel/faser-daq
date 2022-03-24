/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#pragma once

#include "Commons/FaserProcess.hpp"
#include "TrackerReadout/TRBAccess.h"
#include "TrackerCalibration/Module.h"
#include "TrackerCalibration/Chip.h"
#include "TrackerCalibration/ITest.h"

#include <string>
#include <vector>

#include "nlohmann/json.hpp"


class TrackerCalibrationModule : public FaserProcess {
 public:
  TrackerCalibrationModule(const std::string& n);
  ~TrackerCalibrationModule();

  void configure(); // optional (configuration can be handled in the constructor)
  void start(unsigned);
  void stop();

  void runner() noexcept;

 private:
  // tcalib parameters
  
  std::string m_configLocation;
  std::vector<int> m_testSequence;
  std::string m_outBaseDir{"."};
  int m_verboseLevel{0};
  unsigned int m_l1delay{130};
  std::string m_log{"log.txt"};
  bool m_emulateTRB{false};
  bool m_calLoop{false};
  bool m_saveDaq{false};
  bool m_noTrim{false};
  bool m_usb{false};
  std::string m_ip{"10.11.65.8"};

  uint8_t m_globalMask{0x00}; 

  // calibmanager 
  FASER::TRBAccess *m_trb{nullptr};
  std::vector<TrackerCalib::Module*> m_modList; // vector of modules
  std::vector<TrackerCalib::ITest*> m_testList; // vector of tests
  unsigned m_testcnt{1};
  TrackerCalib::ITest *m_t;

  bool m_started{false};
  
};

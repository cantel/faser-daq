/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#pragma once

#include "Commons/FaserProcess.hpp"
#include "TrackerReadout/TRBAccess.h"
#include "TrackerReceiverModule.hpp" 
#include "GPIOBase/DummyInterface.h"
//#include "Commons/EventFormat.hpp"
//#include "Commons/RawExampleFormat.hpp"
#include "TrackerReadout/ConfigurationHandling.h"
#include <string>
#include <iostream>
#include <bitset>
#include <ers/Issue.h>

ERS_DECLARE_ISSUE(TrackerReceiverIssues,                                                             // Namespace
                  NoBoardID,                                                   // Class name
                  "No board ID specified.", // Message
                  ERS_EMPTY)                      // Args

ERS_DECLARE_ISSUE(TrackerReceiverIssues,                                                             // Namespace
                  NoDaqIP,                                                   // Class name
                  "No DAQ IP specified.", // Message
                  ERS_EMPTY)                      // Args

ERS_DECLARE_ISSUE(TrackerReceiverIssues,                                                             // Namespace
                  MissingConfigurationFile,                                                   // Class name
                  "Cannot configure module due to missing configuration file. - Empty configuration file provided for module "<<l_moduleNo<<".", // Message
                  ((unsigned)l_moduleNo))                      // Args

ERS_DECLARE_ISSUE(TrackerReceiverIssues,                                                             // Namespace
                  TLBSyncFailed,                                                   // Class name
                  "Could not sync to TLB CLK", // Message
                  ERS_EMPTY)                      // Args
ERS_DECLARE_ISSUE(TrackerReceiverIssues,                                                             // Namespace
                  TRBConfigurationIssue,                                                   // Class name
                  "Caught TRBConfigurationException with message:"<<std::endl<<message, // Message
                  ((std::string)message))                      // Args

ERS_DECLARE_ISSUE(TrackerReceiverIssues,                                                             // Namespace
                  TRBAccesIssue,                                                   // Class name
                  "Caught TRBAccesException with message:"<<std::endl<<message, // Message
                  ((std::string)message))                      // Args

class TrackerReceiverModule : public FaserProcess {
 public:
  TrackerReceiverModule();
  ~TrackerReceiverModule();

  void configure(); // optional (configuration can be handled in the constructor)
  void start(unsigned);
  void stop();
  void sendECR() override;
  void runner() noexcept;
  void disableTrigger(const std::string &) override;
  void enableTrigger(const std::string &) override;
  
 private:
  std::unique_ptr<FASER::TRBAccess> m_trb;
  unsigned int m_finePhaseDelay_Led0;
  unsigned int m_finePhaseDelay_Clk0;
  unsigned int m_hwDelay_Clk0;
  unsigned int m_finePhaseDelay_Led1;
  unsigned int m_finePhaseDelay_Clk1;
  unsigned int m_hwDelay_Clk1;
  unsigned int m_userBoardID;
  std::string m_SCIP;
  std::string m_DAQIP;
  bool m_extClkSelect;
  FASER::TRBAccess::ABCD_ReadoutMode m_ABCD_ReadoutMode;
  bool m_ABCD_EdgeDetect;
  bool m_configureModules;
  bool m_RxTimeoutDisable;
  unsigned int m_moduleMask;
  unsigned int m_moduleClkCmdMask;
  bool m_triggerEnabled;

  std::atomic<int> m_event_size_bytes;
  std::atomic<int> m_physicsEventCount;
  std::atomic<int> m_corrupted_fragments;
  std::atomic<int> m_checksum_mismatches;
  std::atomic<float> m_checksum_mismatches_rate;
  std::atomic<int> m_number_of_decoded_events;
  std::atomic<int> m_receivedEvents;
  std::atomic<float> m_dataRate;
  std::atomic<int> m_PLLErrCnt;

  bool m_debug = false;
  bool m_trace = false;
  const uint32_t m_TRBENDOFDAQ = 0x07000eee;
  const int m_UPDATEMETRIC_INTERVAL = 10; // in seconds
};

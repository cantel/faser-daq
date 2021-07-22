/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#pragma once

// helper functions that do the actual communication with the digitizer
#include "Comm_vx1730.h" 

// needed for external tools
#include "Commons/FaserProcess.hpp"
#include "EventFormats/DAQFormats.hpp"
#include <ers/Issue.h>

// needed for sendECR() and runner() protection
#include <mutex>

ERS_DECLARE_ISSUE(
DigitizerReceiver,                                                              // namespace
    DigitizerHardwareIssue,                                                    // issue name
  ERS_EMPTY,  // message
  ERS_EMPTY
)
ERS_DECLARE_ISSUE_BASE(DigitizerReceiver,                                          // namespace name
      HostNameOrIPTooLong,                                                  // issue name
      DigitizerReceiver::DigitizerHardwareIssue,                                                // base issue name
      "This is too long of a name: "<<ip<< "Max length of IP hostname: "<<len,                                 // message
      ERS_EMPTY,            // base class attributes
      ((std::string) ip)
      ((unsigned)len)                                     // this class attributes
)
ERS_DECLARE_ISSUE_BASE(DigitizerReceiver,                                          // namespace name
      InvalidIP,                                                  // issue name
      DigitizerReceiver::DigitizerHardwareIssue,                                                // base issue name
      "Invalid IP address: "<<ip,                                 // message
      ERS_EMPTY,            // base class attributes
      ((std::string) ip)                                     // this class attributes
)




class DigitizerReceiverModule : public FaserProcess {
 public:
  DigitizerReceiverModule(const std::string&);
  ~DigitizerReceiverModule();

  ///////////////////////////////////////////
  // Methods needed for FASER/DAQ
  ///////////////////////////////////////////
  void configure(); // optional (configuration can be handled in the constructor)
  void start(unsigned int);
  void stop();
  void sendECR();
  void runner() noexcept;

  ///////////////////////////////////////////
  // Digitizer specific methods and members
  ///////////////////////////////////////////

  // the digitizer hardware accessor object
  vx1730 *m_digitizer;
  
  // picked up in config and used elsewhere  
  std::string m_readout_method;
  int         m_readout_blt;
  
  // the maximum number of events to read out
  int m_n_events_requested;

  // stores monitoring information to be written to histograms/rates
  std::map<std::string, float> m_monitoring;
  
  // used to protect against the ECR and runner() from reading out at the same time
  std::mutex m_lock;

  // used for the trigger time to BCID conversion
  float m_ttt_converter;
  
  // software BCID fix for the digitizer
  float m_bcid_ttt_fix;
  
  // monitoring metrics
  std::atomic<int> m_triggers;
  
  std::atomic<int> m_pedestal[16];
  std::atomic<int> m_temp_ch00;
  std::atomic<int> m_temp_ch01;
  std::atomic<int> m_temp_ch02;
  std::atomic<int> m_temp_ch03;
  std::atomic<int> m_temp_ch04;
  std::atomic<int> m_temp_ch05;
  std::atomic<int> m_temp_ch06;
  std::atomic<int> m_temp_ch07;
  std::atomic<int> m_temp_ch08;
  std::atomic<int> m_temp_ch09;
  std::atomic<int> m_temp_ch10;
  std::atomic<int> m_temp_ch11;
  std::atomic<int> m_temp_ch12;
  std::atomic<int> m_temp_ch13;
  std::atomic<int> m_temp_ch14;
  std::atomic<int> m_temp_ch15;
  
  std::atomic<int> m_hw_buffer_space;
  std::atomic<int> m_hw_buffer_occupancy;
  
  std::atomic<float> m_time_read;
  std::atomic<float> m_time_parse;
  std::atomic<float> m_time_overhead;
  
  std::atomic<int> m_corrupted_events;
  std::atomic<int> m_empty_events;

  std::atomic<int> m_info_udp_receive_timeout_counter;
  std::atomic<int> m_info_wrong_cmd_ack_counter;
  std::atomic<int> m_info_wrong_received_nof_bytes_counter;
  std::atomic<int> m_info_wrong_received_packet_id_counter;

  std::atomic<int> m_info_clear_UdpReceiveBuffer_counter;
  std::atomic<int> m_info_read_dma_packet_reorder_counter;

  std::atomic<int> m_udp_single_read_receive_ack_retry_counter;
  std::atomic<int> m_udp_single_read_req_retry_counter;

  std::atomic<int> m_udp_single_write_receive_ack_retry_counter;
  std::atomic<int> m_udp_single_write_req_retry_counter;

  std::atomic<int> m_udp_dma_read_receive_ack_retry_counter;
  std::atomic<int> m_udp_dma_read_req_retry_counter;

  std::atomic<int> m_udp_dma_write_receive_ack_retry_counter;
  std::atomic<int> m_udp_dma_write_req_retry_counter;

  std::atomic<int> m_bobr_statusword;
  std::atomic<int> m_bobr_timing;
  std::atomic<int> m_lhc_turncount;
  std::atomic<int> m_lhc_fillnumber;
  std::atomic<int> m_lhc_machinemode;
  std::atomic<float> m_lhc_beamenergy;
  std::atomic<int> m_lhc_intensity1;
  std::atomic<int> m_lhc_intensity2;
  std::atomic<float> m_lhc_frequency;

  unsigned int m_prev_seconds;
  int m_prev_microseconds;
  unsigned int m_prev_turncount;

  uint64_t m_prev_event_id;

  // for SW trigger sending
  int m_sw_count;
  float m_software_trigger_rate;

  bool m_bobr;
};

/**
 * Copyright (C) 2019 CERN
 * 
 * DAQling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * DAQling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with DAQling. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

// helper functions that do the actual communication with the digitizer
#include "Comm_vx1730.h" 

// needed for external tools
#include "Commons/FaserProcess.hpp"
#include "EventFormats/DAQFormats.hpp"
#include "Exceptions/Exceptions.hpp"

// needed for sendECR() and runner() protection
#include <mutex>

class DigitizerReceiverException : public Exceptions::BaseException { using Exceptions::BaseException::BaseException; };


class DigitizerReceiverModule : public FaserProcess {
 public:
  DigitizerReceiverModule();
  ~DigitizerReceiverModule();

  void configure(); // optional (configuration can be handled in the constructor)
  void start(unsigned int);
  void stop();
  void sendECR();
  void runner();

  // send a singleEvent from the front of the buffer 
  void sendEventSingle();
  
  // send a batch of events
  // this is more efficient and sendEventSingle() is saved here for completeness
  // we should strive to be rid of it once we are more confident
  void sendEventBatchSpaceSaver(uint32_t raw_payload[], int software_buffer, std::map<std::string, float>& monitoring, int nevents, int nchannels_enabled, int buffer_size, std::string readout_method, int events_to_readout, bool debug=false);

  // used for parsing the software buffer after reading from digitizer
  // stores a single event in single_event_raw_payload and gives you the starting point of the next event as a return value
  int getSingleEvent( uint32_t raw_payload[], uint32_t single_event_raw_payload[], int eventLocation, int eventSize);
  
  // the digitizer hardware accessor object
  vx1730 *m_digitizer;
  
  // raw data payload
  //unsigned int* m_raw_payload;
  int m_software_buffer;
  
  // channel and buffer info
  int m_nchannels_enabled;
  int m_buffer_size;

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
  
  // monitoring metrics
  std::atomic<int> m_triggers;
  
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


  // for SW trigger sending
  int m_sw_count;
  bool m_software_trigger_enable;
  float m_software_trigger_rate;
  
};

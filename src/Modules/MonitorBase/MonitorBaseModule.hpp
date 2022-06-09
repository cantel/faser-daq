/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include <tuple>
#include <list>

#include "Commons/FaserProcess.hpp"
#include "EventFormats/DAQFormats.hpp"
// import all existing data formats
#include "EventFormats/RawExampleFormat.hpp"
#include "EventFormats/TLBDataFragment.hpp"
#include "EventFormats/TLBMonitoringFragment.hpp"
#include "EventFormats/DigitizerDataFragment.hpp"
#include "EventFormats/TrackerDataFragment.hpp"
#include "EventFormats/BOBRDataFragment.hpp"

#include "Utils/HistogramManager.hpp"
#include "Utils/Ers.hpp"
#include "Exceptions/Exceptions.hpp"
#include <Utils/Binary.hpp>

using namespace DAQFormats;
using namespace TLBDataFormat;
using namespace TLBMonFormat;
using namespace BOBRDataFormat;

ERS_DECLARE_ISSUE(
MonitorBase,                                                              // namespace
    ConfigurationIssue,                                                    // issue name
  message,  // message
    ((std::string) message)
)
ERS_DECLARE_ISSUE(
MonitorBase,                                                              // namespace
    UnpackDataIssue,                                                    // issue name
  message,  // message
    ((std::string) message)
)

class MonitorBaseModule : public FaserProcess {
 public:
  MonitorBaseModule(const std::string&);
  virtual ~MonitorBaseModule();

  void configure();
  void start(unsigned int);
  void stop();
  void runner() noexcept;



 protected:

  // filled by json configs
  uint32_t m_sourceID=0;
  uint8_t m_eventTag;
  bool m_filter_physics;
  bool m_filter_random;
  bool m_filter_led;
  bool m_store_bobr_data;
  unsigned m_PUBINT;
  
  EventFull* m_event=0;
  const EventFragment* m_fragment=0; // do not delete this one. Owned by m_event!
  const RawFragment * m_rawFragment = 0 ; // do not delete this one. Owned by m_event!
  const MonitoringFragment * m_monitoringFragment = 0 ; // ""
  std::unique_ptr<TLBMonitoringFragment> m_tlbmonitoringFragment;
  std::unique_ptr<TLBDataFragment> m_tlbdataFragment;
  std::unique_ptr<DigitizerDataFragment> m_pmtdataFragment;
  const TrackerDataFragment * m_trackerdataFragment = 0;

  // histogramming
  bool m_histogramming_on;
  std::unique_ptr<HistogramManager> m_histogrammanager;

  std::atomic<int> m_metric_payload;
  std::atomic<int> m_metric_error_unclassified;
  std::atomic<int> m_metric_error_bcidmismatch;
  std::atomic<int> m_metric_error_tagmismatch;
  std::atomic<int> m_metric_error_timeout;
  std::atomic<int> m_metric_error_overflow;
  std::atomic<int> m_metric_error_corrupted;
  std::atomic<int> m_metric_error_dummy;
  std::atomic<int> m_metric_error_missing;
  std::atomic<int> m_metric_error_empty;
  std::atomic<int> m_metric_error_duplicate;
  std::atomic<int> m_metric_error_unpack;

  // BOBR data to be stored
  std::atomic<int> m_lhc_machinemode;

  // functions 
  virtual void monitor(DataFragment<daqling::utilities::Binary>&);
  virtual void register_hists( );
  virtual void register_metrics();
  void register_error_metrics(); // lets derived classes register metrics for all error types defined in EventFormat.
  uint16_t unpack_event_header( DataFragment<daqling::utilities::Binary> &eventBuilderBinary );
  uint16_t unpack_fragment_header( DataFragment<daqling::utilities::Binary> &eventBuilderBinary, uint32_t sourceID);
  uint16_t unpack_fragment_header( DataFragment<daqling::utilities::Binary> &eventBuilderBinary);
  uint16_t unpack_full_fragment( DataFragment<daqling::utilities::Binary> &eventBuilderBinary, uint32_t sourceID);
  TrackerDataFragment get_tracker_data_fragment(DataFragment<daqling::utilities::Binary> &eventBuilderBinary, uint8_t boardId);
  DigitizerDataFragment get_digitizer_data_fragment(DataFragment<daqling::utilities::Binary> &eventBuilderBinary);
  TLBDataFragment get_tlb_data_fragment(DataFragment<daqling::utilities::Binary> &eventBuilderBinary);
  TLBMonitoringFragment get_tlb_monitoring_fragment(DataFragment<daqling::utilities::Binary> &eventBuilderBinary);
  uint16_t unpack_full_fragment( DataFragment<daqling::utilities::Binary> &eventBuilderBinary);
  void fill_error_status_to_metric(uint32_t fragmentStatus);
  void fill_error_status_to_histogram(uint32_t fragmentStatus, std::string hist_name);
  bool is_physics_triggered(DataFragment<daqling::utilities::Binary>&);
  bool is_random_triggered(DataFragment<daqling::utilities::Binary>&);
  bool is_led_triggered(DataFragment<daqling::utilities::Binary>&);
  

 private:

  std::thread *m_bobrProcessThread;
  bool m_event_header_unpacked;
  const int m_INTERVAL_BOBRUPDATE = 5; // in seconds
  std::vector<int> m_active_mon_lhc_modes;
  bool m_activate_monitoring;

  void setupHistogramManager();
  // store BOBR info
  void process_bobr_data() noexcept;
  uint16_t store(DataFragment<daqling::utilities::Binary>&);

};

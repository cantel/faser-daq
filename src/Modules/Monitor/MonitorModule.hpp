#pragma once

#include <tuple>
#include <list>

#include "Core/DAQProcess.hpp"
#include "Commons/EventFormat.hpp"
#include "Utils/HistogramManager.hpp"
#include "Utils/Logging.hpp"

class MonitorModule : public daqling::core::DAQProcess {
 public:
  MonitorModule();
  virtual ~MonitorModule();

  void start();
  void stop();
  void runner();



 protected:

  // filled by json configs
  uint32_t m_sourceID;
  
  EventHeader * m_eventHeader = new EventHeader;
  EventFragmentHeader * m_fragmentHeader = new EventFragmentHeader;
  RawFragment * m_rawFragment = new RawFragment;
  const size_t m_eventHeaderSize = sizeof(EventHeader);
  const size_t m_fragmentHeaderSize = sizeof(EventFragmentHeader) ;
  const size_t m_rawFragmentSize = sizeof(RawFragment) ;

  // histogramming
  bool m_histogramming_on;
  std::unique_ptr<HistogramManager> m_histogrammanager;

  std::atomic<int> m_metric_payload;
  std::atomic<int> m_metric_error_ok;
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

  // functions 
  virtual void monitor(daqling::utilities::Binary&);
  virtual void register_hists( );
  virtual void register_metrics();
  void register_error_metrics(std::string module_short_name); // lets derived classes register metrics for all error types defined in EventFormat.
  uint16_t unpack_event_header( daqling::utilities::Binary &eventBuilderBinary );
  uint16_t unpack_fragment_header( daqling::utilities::Binary &eventBuilderBinary );
  uint16_t unpack_full_fragment( daqling::utilities::Binary &eventBuilderBinary );
  void fill_error_status_to_metric(uint32_t fragmentStatus);
  void fill_error_status_to_histogram(uint32_t fragmentStatus, std::string hist_name);

 private: // CHANGE TO PRIVATE

  bool m_event_header_unpacked;
  bool m_fragment_header_unpacked;
  bool m_raw_fragment_unpacked;

  void setupHistogramManager();

};

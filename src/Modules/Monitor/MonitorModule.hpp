#pragma once

#include <tuple>
#include <list>

#include "Core/DAQProcess.hpp"
#include "Commons/EventFormat.hpp"
#include "Utils/HistogramManager.hpp"

class MonitorModule : public daqling::core::DAQProcess {
 public:
  MonitorModule();
  virtual ~MonitorModule();

  void start();
  void stop();

  void runner();



 protected:

  uint32_t m_sourceID;
  const size_t m_eventHeaderSize = sizeof(EventHeader);
  const size_t m_fragmentHeaderSize = sizeof(EventFragmentHeader) ;

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
  void setupHistogramManager();
  virtual void register_hists( );
  virtual void register_metrics();
  uint16_t unpack_data( daqling::utilities::Binary &eventBuilderBinary, EventHeader *& eventHeader, EventFragmentHeader *& fragmentHeader );
  void fill_error_status_to_metric(uint32_t fragmentStatus);
  void fill_error_status_to_histogram(uint32_t fragmentStatus, std::string hist_name);
};

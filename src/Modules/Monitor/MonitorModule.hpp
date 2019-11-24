#pragma once

#include <tuple>
#include <list>

#include "Commons/FaserProcess.hpp"
#include "Commons/EventFormat.hpp"
#include "Commons/RawExampleFormat.hpp"
#include "Utils/HistogramManager.hpp"
#include "Utils/Logging.hpp"

class MonitorModule : public FaserProcess {
 public:
  MonitorModule();
  virtual ~MonitorModule();

  void start(int);
  void stop();
  void runner();



 protected:

  // filled by json configs
  uint32_t m_sourceID=0;
  
  EventFull* m_event=0;
  const EventFragment* m_fragment=0; // do not delete this one. Owned by m_event!
  const RawFragment * m_rawFragment = 0 ; // do not delete this one. Owned by m_event!

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
  uint16_t unpack_fragment_header( daqling::utilities::Binary &eventBuilderBinary, uint32_t sourceID);
  uint16_t unpack_fragment_header( daqling::utilities::Binary &eventBuilderBinary);
  uint16_t unpack_full_fragment( daqling::utilities::Binary &eventBuilderBinary, uint32_t sourceID);
  uint16_t unpack_full_fragment( daqling::utilities::Binary &eventBuilderBinary);
  void fill_error_status_to_metric(uint32_t fragmentStatus);
  void fill_error_status_to_histogram(uint32_t fragmentStatus, std::string hist_name);

 private:

  bool m_event_header_unpacked;

  void setupHistogramManager();

};

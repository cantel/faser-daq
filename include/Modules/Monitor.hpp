#ifndef MONITOR_H_
#define MONITOR_H_

//#include <boost/histogram.hpp>
#include "Modules/Histogram.hpp"
#include "Modules/HistogramManager.hpp"
#include <tuple>
#include "Core/DAQProcess.hpp"
#include <nlohmann/json.hpp>
#include <list>
//#include <cpr/cpr.h>
//#include "zmq.hpp"

#include "Modules/EventFormat.hpp"

class Monitor : public daqling::core::DAQProcess {
 public:
  Monitor();
  virtual ~Monitor();

  void start();
  void stop();

  void runner();



 protected:

  using json = nlohmann::json;

  uint32_t m_sourceID;
  const size_t m_eventHeaderSize = sizeof(EventHeader);
  const size_t m_fragmentHeaderSize = sizeof(EventFragmentHeader) ;

  std::string m_outputdir;
  std::string m_json_file_name;

  // histogramming
  //std::unique_ptr<zmq::socket_t> m_hists_socket;
  //std::unique_ptr<zmq::context_t> m_hists_context;
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

  //std::map< std::string, hist_t> m_hist_map;
  categoryhist_t m_histogram_error; 
  hist_t m_payloadsize_hist; 
 
  bool setupHistogramManager();
  //std::unique_ptr<zmq::socket_t>& getHistSocket() { return std::ref(m_hists_socket); }
  virtual void register_hists( );
  virtual void register_metrics();
  uint16_t unpack_data( daqling::utilities::Binary eventBuilderBinary, const EventHeader *& eventHeader, EventFragmentHeader *& fragmentHeader );
  void fill_error_status_to_metric(uint32_t fragmentStatus);
  void fill_error_status_to_histogram(uint32_t fragmentStatus, std::string hist_name);
  //template <typename T>
  //void flush_hist( T histStruct, bool coverage_all = true );
  //void flush_hist( Hist<categoryhist_t> histStruct, bool coverage_all = false );
  //void flush_hist( Hist<graph_t> histStruct, bool coverage_all = false );
  //template <typename T>
  //void write_hist_to_file( T histStruct, std::string dir, bool coverage_all = true );
  //void write_hist_to_file( Hist<categoryhist_t> histStruct, std::string dir, bool coverage_all = false );
  //template <typename T>
  //void add_hist_to_json( T histStruct, json &jsonArray,bool coverage_all = true );
  //void add_hist_to_json( Hist<categoryhist_t> histStruct, json &jsonArray, bool coverage_all = false );
  //void add_hist_to_json( Hist<graph_t> graphStruct, json &jsonArray, bool coverage_all = false );
  //void write_hists_to_json( HistList hist_lists, bool coverage_all = true );
  //template <typename T>
  //void write_hists_to_json( HistMap<T> hist_lists, bool coverage_all = true );

};

#endif /* MONITOR_H_ */

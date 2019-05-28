#ifndef MONITOR_H_
#define MONITOR_H_

#include <boost/histogram.hpp>
#include <tuple>
#include "Core/DAQProcess.hpp"

class Monitor : public daqling::core::DAQProcess {
 public:
  Monitor();
  ~Monitor();

  void start();
  void stop();

  void runner();

 private:

  using axis_t = boost::histogram::axis::regular<>;
  using hist_t = decltype(boost::histogram::make_histogram(std::declval<axis_t>())); // most hists are of this type
  using categoryaxis_t = boost::histogram::axis::category<std::string>;
  using categoryhist_t = decltype(boost::histogram::make_histogram(std::declval<categoryaxis_t>())); 

   struct Hist {
     std::string histname;
     hist_t hist;
   };
   struct CategoryHist {
     std::string histname;
     categoryhist_t hist;
   };

   // using HistMap storage instead.
  //using regmaphist_t = std::map<std::string, decltype(boost::histogram::make_histogram(std::declval<regaxis_t>()))>; 
  //using catmaphist_t = std::map<std::string, decltype(boost::histogram::make_histogram(std::declval<cataxis_t>()))>; 

   /*
   struct HistMaps {                                                                           
     regmaphist_t reghists;                                                                    
     catmaphist_t cathists;                                                                    
   }; */

  //using histmap_t = std::map<std::string, decltype(boost::histogram::make_histogram(std::declval<axis_t>()))>;
 //using histmap_t = boost::variant<std::map<std::string, decltype(boost::histogram::make_histogram(std::declval<regaxis_t>()))>, std::map<std::string, decltype(boost::histogram::make_histogram(std::declval<cataxis_t>()))>>;
  //HistMaps m_histMap;
  
  // ###################################################################################
  // -- list of hists --							       #
  // Hist is hist with regular axis ( axis with constant bin width along real line ).  #
  // choose CategoryHist for hists with bin value of string type.		       #
  // storage is boost::histogram's default.					       #
  Hist h_timedelay_rcv1_rcv2 = { "h_timedelay_rcv1_rcv2" };
  Hist h_payloadsize_rcv1 = { "h_payloadsize_rcv1" };
  Hist h_payloadsize_rcv2 = { "h_payloadsize_rcv2" };
  CategoryHist h_fragmenterrors = { "h_fragmenterrors" };

  uint64_t m_timeDelayTolerance; 

  void initialize_hists( );
  template <typename T>
  void flush_hist( T histStruct, bool coverage_all = true );
  void flush_hist( CategoryHist histStruct, bool coverage_all = false );
  template <typename T>
  void write_hist_to_file( T histStruct, std::string dir, bool coverage_all = true );
  void write_hist_to_file( CategoryHist histStruct, std::string dir, bool coverage_all = false );
};

#endif /* MONITOR_H_ */

#ifndef MONITOR_H_
#define MONITOR_H_

//#include <boost/histogram.hpp>
#include "Modules/Histogram.hpp"
#include <tuple>
#include "Core/DAQProcess.hpp"
#include <nlohmann/json.hpp>
#include <list>

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

   // using HistMap storage instead.
  using reghistmap_t = std::map<std::string, RegularHist>; 
  using cathistmap_t = std::map<std::string, CategoryHist>; 
  using graphmap_t = std::map<std::string, Graph>; 

   struct HistMaps {                                                                           
     reghistmap_t reghistmap;                                                                    
     cathistmap_t cathistmap;                                                                    
     graphmap_t graphmap;                                                                    

     void addHist( std::string hist_name, RegularHist hist ) {
	reghistmap[hist_name] = hist;
     }
     void addHist( std::string hist_name, CategoryHist hist ) {
	cathistmap[hist_name] = hist;
     }
     void addHist( std::string hist_name, Graph hist ) {
	graphmap[hist_name] = hist;
     }

    void fillHist( std::string hist_name, float value ) {
	if ( reghistmap.count(hist_name) ) { 
	    reghistmap[hist_name].object(value);
	}
	else ERROR( " Histogram with name "<<hist_name<<" not found ! ");
    }
    void fillHist( std::string hist_name, std::string value ) {
	if ( cathistmap.count(hist_name) ) { 
	   cathistmap[hist_name].object(value);
	}
	else ERROR( " Histogram with name "<<hist_name<<" not found ! ");
    }
    void fillHist( std::string hist_name, std::pair<unsigned int, unsigned int> xy) {
	if ( graphmap.count(hist_name) ) { 
	    graphmap[hist_name].object.push_back( xy );
	}
	else ERROR( " Histogram with name "<<hist_name<<" not found ! ");
    }
    void fillHist( std::string hist_name, unsigned int x, unsigned int y) {
	if ( graphmap.count(hist_name) ) { 
	    graphmap[hist_name].object.push_back( std::make_pair(x, y));
	}
	else ERROR( " Histogram with name "<<hist_name<<" not found ! ");
    }
   };

  // ###################################################################################
  // -- list of hists --							       #
  // Hist is hist with regular axis ( axis with constant bin width along real line ).  #
  // choose CategoryHist for hists with bin value of string type.		       #
  // storage is boost::histogram's default.					       #
  // hist
  //RegularHist h_timedelay_rcv1_rcv2 = { "h_timedelay_rcv1_rcv2", "time delay [micro s]"};

  struct HistList {
    std::vector<RegularHist*> histlist;
    std::vector<CategoryHist*> categoryhistlist; 
    std::vector<Graph*> graphlist;

    void addHist( RegularHist * histPtr ) {	
	histlist.push_back( histPtr );
    }
    void addHist( CategoryHist * histPtr ) {
	categoryhistlist.push_back( histPtr );
    }
    void addHist( Graph * histPtr ) {
	graphlist.push_back( histPtr );
    }
  };

  uint32_t m_sourceID;
  const size_t m_eventHeaderSize = sizeof(EventHeader);
  const size_t m_fragmentHeaderSize = sizeof(EventFragmentHeader) ;

  categoryaxis_t m_axis_fragmenterrors = categoryaxis_t({"Ok", "Unclassified", "BCIDMistmatch", "TagMismatch", "Timeout", "Overflow","Corrupted", "Dummy", "Missing", "Empty", "Duplicate", "DataUnpack"}, "error type");
  //HistList m_hist_lists;
  HistMaps m_hist_map;
  std::string m_outputdir;
  std::string m_json_file_name;

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

  virtual void initialize_hists( );
  virtual void register_metrics();
  uint16_t unpack_data( daqling::utilities::Binary eventBuilderBinary, const EventHeader *& eventHeader, EventFragmentHeader *& fragmentHeader );
  void fill_error_status(CategoryHist &hist, uint32_t fragmentStatus);
  void fill_error_status(std::string hist_name, uint32_t fragmentStatus);
  void fill_error_status(uint32_t fragmentStatus);
  template <typename T>
  void flush_hist( T histStruct, bool coverage_all = true );
  void flush_hist( CategoryHist histStruct, bool coverage_all = false );
  void flush_hist( Graph histStruct, bool coverage_all = false );
  template <typename T>
  void write_hist_to_file( T histStruct, std::string dir, bool coverage_all = true );
  void write_hist_to_file( CategoryHist histStruct, std::string dir, bool coverage_all = false );
  template <typename T>
  void add_hist_to_json( T histStruct, json &jsonArray,bool coverage_all = true );
  void add_hist_to_json( CategoryHist histStruct, json &jsonArray, bool coverage_all = false );
  void add_hist_to_json( Graph graphStruct, json &jsonArray, bool coverage_all = false );
  void write_hists_to_json( HistList hist_lists, bool coverage_all = true );
  void write_hists_to_json( HistMaps hist_lists, bool coverage_all = true );

};

#endif /* MONITOR_H_ */

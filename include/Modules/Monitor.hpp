#ifndef MONITOR_H_
#define MONITOR_H_

#include <boost/histogram.hpp>
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

  using axis_t = boost::histogram::axis::regular<>;
  using hist_t = decltype(boost::histogram::make_histogram(std::declval<axis_t>())); // most hists are of this type
  using categoryaxis_t = boost::histogram::axis::category<std::string>;
  using categoryhist_t = decltype(boost::histogram::make_histogram(std::declval<categoryaxis_t>())); 
  using graph_t = std::vector<std::pair<unsigned int, unsigned int>>; // assume for now int vs int is the most common type

   struct Hist {
     Hist() : ylabel("counts") {}
     Hist(std::string name, std::string xlabel) : name(name), title(name), xlabel(xlabel), ylabel("counts") {}
     Hist(std::string name, std::string xlabel, std::string ylabel) : name(name), title(name), xlabel(xlabel), ylabel(ylabel) {}
     std::string name;
     std::string title;
     std::string xlabel;
     std::string ylabel;
     unsigned int binwidth; 
     hist_t object;
     hist_t getObject() { return object; };
   };

   struct RegularHist : Hist {
     RegularHist() : Hist() {};
     RegularHist(std::string name, std::string xlabel) : Hist(name, xlabel){}
     hist_t object;
     hist_t getObject() { return object; };
   };

   struct CategoryHist : Hist {
     CategoryHist() : Hist() {};
     CategoryHist(std::string name, std::string xlabel) : Hist(name, xlabel){}
     categoryhist_t object;
     categoryhist_t getObject() { return object; };
   };

   struct Graph : Hist {
     Graph(){};
     Graph(std::string name, std::string xlabel, std::string ylabel) : Hist(name, xlabel, ylabel){}
     graph_t object;
     graph_t getObject() { return object; };
   };

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
  const size_t m_combinedHeaderSize = m_eventHeaderSize+m_fragmentHeaderSize;

  //HistList m_hist_lists;
  HistMaps m_hist_map;
  unsigned int m_error_rate_cnt;
  uint16_t m_timeblock; //in seconds
  uint64_t m_timeDelayTolerance; 
  std::string m_outputdir;
  std::string m_json_file_name;

  void initialize_hists( );
  bool unpack_data( daqling::utilities::Binary eventBuilderBinary, const EventHeader *& eventHeader, EventFragmentHeader *& fragmentHeader );
  void fill_error_status(CategoryHist &hist, uint32_t fragmentStatus);
  void fill_error_status(std::string hist_name, uint32_t fragmentStatus);
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

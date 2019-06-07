/// \cond
#include <chrono>
/// \endcond
using namespace std::chrono_literals;
using namespace std::chrono;

#include <map>
#include <boost/format.hpp>
#include <boost/histogram/ostream.hpp> // write histogram straight to ostream
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream

using namespace boost::histogram;

#include "Modules/Monitor.hpp"

#define __MODULEMETHOD_NAME__ daqling::utilities::methodName(__PRETTY_FUNCTION__)
#define __MODULECLASS_NAME__ daqling::utilities::className(__PRETTY_FUNCTION__)

extern "C" Monitor *create_object() { return new Monitor; }

extern "C" void destroy_object(Monitor *object) { delete object; }

Monitor::Monitor() { 

   INFO("Monitor::Monitor");

   //initialize_hists( m_histMap );
   initialize_hists( );

   // make this configurable ...?
   m_json_file_name = "test_histogram_output.json";

   auto cfg = m_config.getConfig()["settings"];
   m_timeblock = cfg["lengthTimeBlock"]; // seconds
   m_outputdir = cfg["outputDir"];
   m_timeDelayTolerance = cfg["timeDelayTolerance"]; // microseconds

 }

Monitor::~Monitor() { 
  INFO(__MODULEMETHOD_NAME__ << " With config: " << m_config.dump() << " getState: " << this->getState());
 }

void Monitor::start() {
  DAQProcess::start();
  INFO(__MODULEMETHOD_NAME__ << " getState: " << this->getState());

}

void Monitor::stop() {
  DAQProcess::stop();

  write_hists_to_json( m_hist_lists );

  INFO(__MODULEMETHOD_NAME__ << " getState: " << this->getState());
}

void Monitor::runner() {
  INFO(__MODULEMETHOD_NAME__ << " Running...");

  bool isData(true);

  m_error_rate_cnt = 0;
  seconds timestamp_in_seconds;
  bool update = true;
  auto starting_time_in_seconds = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
 
  while (m_run) {
    daqling::utilities::Binary eventBuilderBinary;

    isData = m_connections.get(1, eventBuilderBinary);
    if ( !isData ) std::this_thread::sleep_for(10ms);
    else {
        INFO(__MODULEMETHOD_NAME__ <<  " isData "<< isData );
        auto dataTotalSize = eventBuilderBinary.size();

        const EventHeader * eventHeader((EventHeader *)malloc(m_eventHeaderSize));
        eventHeader = static_cast<const EventHeader *>(eventBuilderBinary.data());
        uint16_t numChannels = eventHeader->num_fragments;
        uint32_t totalPayloadSize = eventBuilderBinary.size();

        std::map<uint32_t, EventFragmentHeader*> fragmentHeaders;
        std::map<uint32_t, uint16_t> payloadSizes;
        std::map<uint32_t, uint64_t> timeStamps;
        uint32_t accumulatedPayloadSize = 0;

        INFO(__MODULEMETHOD_NAME__ <<  " about to loop through "<< numChannels << " channels." ); 
        for ( int chno=0; chno<numChannels; ++chno){

            int ch = chno + 1;

            INFO(__MODULEMETHOD_NAME__ <<  " channel number "<< ch ); 

            EventFragmentHeader * fragmentHeader((EventFragmentHeader *)malloc(m_fragmentHeaderSize));
            memcpy( fragmentHeader, static_cast<const char*>(eventBuilderBinary.data())+m_eventHeaderSize+m_fragmentHeaderSize*(ch-1)+accumulatedPayloadSize, m_fragmentHeaderSize);

            fragmentHeaders[ch] = fragmentHeader;	

            uint16_t payloadSize = fragmentHeader->payload_size; 
            payloadSizes[ch] = payloadSize;
	    uint32_t fragmentStatus = fragmentHeader->status;
            timeStamps[ch] = fragmentHeader->timestamp;
            
            accumulatedPayloadSize += payloadSize;
            INFO(__MODULEMETHOD_NAME__ <<  " accumulated payload size "<<accumulatedPayloadSize ); 
       
            EventFragment * payload((EventFragment *)malloc(payloadSize));
            memcpy( payload, static_cast<const char*>(eventBuilderBinary.data())+m_combinedHeaderSize, payloadSize);

	    // fill per channel hists.
	    if ( fragmentStatus == 0 ) h_fragmenterrors.object( "Ok");
	    else {
	    m_error_rate_cnt+=1;
	    if ( fragmentStatus & CorruptedFragment ) h_fragmenterrors.object( "Corrupted");
	    if ( fragmentStatus & EmptyFragment ) h_fragmenterrors.object( "Empty");
	    if ( fragmentStatus & MissingFragment ) h_fragmenterrors.object( "Missing");
	    if ( fragmentStatus & BCIDMismatch ) h_fragmenterrors.object( "BCIDMismatch");
	    }
        }

        auto totalCalculatedSize = m_eventHeaderSize+m_fragmentHeaderSize*numChannels+accumulatedPayloadSize ;
	
	// sanity check
	if ( totalCalculatedSize != totalPayloadSize ) 
	ERROR(__MODULEMETHOD_NAME__ <<  "total byte size unpacked, "
		              << totalCalculatedSize 
                              <<", not equal to the total payload size, "
                              <<totalPayloadSize
                              <<" given in header. FIX ME." );

        if ( (timeStamps.find(1) == timeStamps.end()) || (timeStamps.find(2) == timeStamps.end()) ) {
        	ERROR(__MODULEMETHOD_NAME__ <<  "not all channels found. ");
        }
        else{
        	int64_t timedelay = timeStamps[1] - timeStamps[2];
        	h_timedelay_rcv1_rcv2.object( timedelay );
		INFO(" time delay "<< timedelay );
        }
        if ( !(payloadSizes.find(1) == payloadSizes.end()) ) {
		INFO(" filling payload 1 size "<< payloadSizes[1] );
		h_payloadsize_rcv1.object( payloadSizes[1]);
        }
	else ERROR(__MODULEMETHOD_NAME__ <<  "did not find channel 1.");
        if ( !(payloadSizes.find(2) == payloadSizes.end()) ) {
		INFO(" filling payload 1 size "<< payloadSizes[2] );
		h_payloadsize_rcv2.object( payloadSizes[2]);
        }
	else ERROR(__MODULEMETHOD_NAME__ <<  "did not find channel 2.");

        timestamp_in_seconds = duration_cast<seconds>(system_clock::now().time_since_epoch());
	if ( (timestamp_in_seconds.count() - starting_time_in_seconds)%m_timeblock == 0){
	    if ( update ) { 
	    g_error_rate_per_timeblock.object.push_back( std::make_pair(timestamp_in_seconds.count(), m_error_rate_cnt));
	    m_error_rate_cnt = 0;
	    update = false;
	    }
 	}
	else update = true;


    }
  }
  
  INFO(__MODULEMETHOD_NAME__ << " flushing h_payloadsize_rcv1");
  flush_hist( h_payloadsize_rcv1 );
  INFO(__MODULEMETHOD_NAME__ << " flushing h_payloadsize_rcv2");
  flush_hist( h_payloadsize_rcv2 );
  INFO(__MODULEMETHOD_NAME__ << " flushing h_timedelay_rcv1_rcv2");
  flush_hist( h_timedelay_rcv1_rcv2 );
  INFO(__MODULEMETHOD_NAME__ << " flushing h_fragmenterrors");
  flush_hist( h_fragmenterrors, false );
  INFO(__MODULEMETHOD_NAME__ << " flushing g_error_rate_per_timeblock");
  flush_hist( g_error_rate_per_timeblock, false );

   // write out
  //write_hist_to_file( h_payloadsize_rcv2, m_outputdir);
  //write_hist_to_file( h_payloadsize_rcv1, m_outputdir);
  //write_hist_to_file( h_timedelay_rcv1_rcv2, m_outputdir); 
  //write_hist_to_file( h_fragmenterrors, m_outputdir); 


  INFO(__MODULEMETHOD_NAME__ << " Runner stopped");
}

//void Monitor::initialize_hists( HistMaps &histMap ) {
void Monitor::initialize_hists( ) {

  // histograms
  
    INFO("making h_timedelay_rcv1_rcv2"); 
  h_timedelay_rcv1_rcv2.object = make_histogram(axis::regular<>(100, -500., 500., "time [mus]"));
  m_hist_lists.addHist(&h_timedelay_rcv1_rcv2);
    INFO("making h_payloadsize_rcv1"); 
  h_payloadsize_rcv1.object = make_histogram(axis::regular<>(275, -0.5, 545.5, "payload size"));
  m_hist_lists.addHist(&h_payloadsize_rcv1);
    INFO("making h_payloadsize_rcv2");
  h_payloadsize_rcv2.object = make_histogram(axis::regular<>(275, -0.5, 545.5, "payload size"));
  m_hist_lists.addHist(&h_payloadsize_rcv2);
    INFO("making h_fragmenterrors");
  auto axis_fragmenterrors = axis::category<std::string>({"Ok", "Corrupted","Empty", "Missing", "BCIDMismatch"}, "error type");
  h_fragmenterrors.object = make_histogram(axis_fragmenterrors);
  m_hist_lists.addHist(&h_fragmenterrors);

  // graphs
  m_hist_lists.addHist(&g_error_rate_per_timeblock);
  
  //
  // using HistMap storage instead.
    /*
    INFO("making h_timedelay_rcv1_rcv2");
    histMap.reghists["h_timedelay_rcv1_rcv2"] = make_histogram(axis::regular<>(100, -500., 500., "time [mus]"));
    INFO("making h_payloadsize_rcv1");
    histMap.reghists["h_payloadsize_rcv1"] = make_histogram(axis::regular<>(275, -0.5, 545.5, "payload size"));
    INFO("making h_payloadsize_rc2");
    histMap.reghists["h_payloadsize_rcv2"] = make_histogram(axis::regular<>(275, -0.5, 545.5, "payload size"));

    INFO("making h_fragmenterrors");
    auto axis_fragmenterrors = axis::category<std::string>({"Corrupted","Empty", "Missing", "BCIDMismatch"}, "error type");
    histMap.cathists["h_fragmenterrors"] = make_histogram(axis_fragmenterrors);
    */

    return ;

}

template <typename T>
void Monitor::flush_hist( T histStruct, bool coverage_all ) {

  INFO(__MODULEMETHOD_NAME__ << " flushing hist info for "<< histStruct.name);

  std::ostringstream os;
  auto hist = histStruct.getObject();
  auto this_axis = hist.axis();
  auto thiscoverage = coverage::all;
  if ( !coverage_all ) thiscoverage = coverage::inner;

   for (auto x : indexed(hist, thiscoverage) ) { 
      os << boost::format("bin %2i bin value %s : %i\n") % x.index() % this_axis.value(x.index()) % *x;
   }

  std::cout << os.str() << std::flush;

  return ;

}

void Monitor::flush_hist( CategoryHist histStruct, bool coverage_all ) {

  INFO(__MODULEMETHOD_NAME__ << " flushing hist info for "<<histStruct.name);
  INFO(__MODULEMETHOD_NAME__ << " hist axis is of type category; hist has no under/overflow bins. ");

  std::ostringstream os;
  auto hist = histStruct.getObject();
  auto this_axis = hist.axis();

   for (auto x : indexed(hist, coverage::inner) ) { // no under/overflow bins for category axis. 
      os << boost::format("bin %2i bin value %s : %i\n") % x.index() % this_axis.value(x.index()) % *x;
   }

  std::cout << os.str() << std::flush;

  return ;

}

void Monitor::flush_hist( Graph histStruct, bool coverage_all ) {

  INFO(__MODULEMETHOD_NAME__ << " flushing hist info for "<<histStruct.name);

  std::ostringstream os;
  auto hist = histStruct.getObject();

   for (auto xy : histStruct.getObject()  ) {
      os << boost::format("x:  %2f y: %2f \n") % xy.first % xy.second;
   }

  std::cout << os.str() << std::flush;

  return ;

}

template <typename T>
void Monitor::write_hist_to_file( T histStruct, std::string dir, bool coverage_all ) {

  auto hist = histStruct.getObject(); 
  std::string file_name = dir + "histoutput_"+histStruct.name+".txt";

  INFO(__MODULEMETHOD_NAME__ << " writing hist info  to file ... "<< file_name);

  //std::ostringstream os;
  std::ofstream ofs( file_name );

  ofs << "bin \t bin width \t bin center \t x \n";

  auto this_axis = hist.axis();
  auto thiscoverage = coverage::all;
  if ( !coverage_all ) thiscoverage = coverage::inner;
  for (auto x : indexed(hist, thiscoverage) ) { // no under/overflow bins for category axis. 
      ofs << boost::format("%2i \t %2f \t %2f \t %i\n") % x.index() %(x.bin().upper()-x.bin().lower()) % this_axis.value(x.index()) % *x;
   }

  ofs.close();

  return ;

}

template <typename T>
void Monitor::add_hist_to_json( T * histStruct, json &jsonArray, bool coverage_all ) {


   auto hist = histStruct->getObject();
   
   float binwidth =  hist.axis().bin(1).upper() - hist.axis().bin(2).lower();
 
   json histogram = json::object();
   histogram["name"] = histStruct->name;
   histogram["type"] = "regular";
   histogram["title"] = histStruct->title;
   histogram["xlabel"] = histStruct->xlabel;
   histogram["ylabel"] = histStruct->ylabel;
   histogram["binwidth"] = binwidth;
 
   std::vector<float> xcolumn;
   std::vector<unsigned int> ycolumn;
 
   auto thiscoverage = coverage::all;
   if ( !coverage_all ) thiscoverage = coverage::inner;
   auto this_axis = hist.axis();
 
   for (auto x : indexed(hist, thiscoverage) ) {
       xcolumn.push_back(this_axis.value(x.index())); 
       ycolumn.push_back(*x); 
    }
    
    if ( xcolumn.size() != ycolumn.size() ) {
        ERROR(__MODULEMETHOD_NAME__ << " x and y columns do not have same size. Writing empty vectors to file.");
        xcolumn.clear();
        ycolumn.clear();
    }

    histogram["x"] = xcolumn;
    histogram["y"] = ycolumn;

    jsonArray.push_back( histogram );

    return ;
}

void Monitor::add_hist_to_json( CategoryHist * histStruct, json &jsonArray, bool coverage_all ) {

   auto hist = histStruct->getObject();
  
   json histogram = json::object();
   histogram["name"] = histStruct->name;
   histogram["type"] = "category";
   histogram["title"] = histStruct->title;
   histogram["xlabel"] = histStruct->xlabel;
   histogram["ylabel"] = histStruct->ylabel;
 
   std::vector<std::string> xcolumn;
   std::vector<unsigned int> ycolumn;
 
   auto this_axis = hist.axis();
 
   for (auto x : indexed(hist, coverage::inner) ) {
       xcolumn.push_back(this_axis.value(x.index())); 
       ycolumn.push_back(*x); 
    }
    
    if ( xcolumn.size() != ycolumn.size() ) {
        ERROR(__MODULEMETHOD_NAME__ << " x and y columns do not have same size. Writing empty vectors to file.");
        xcolumn.clear();
        ycolumn.clear();
    }
 
    histogram["x"] = xcolumn;
    histogram["y"] = ycolumn;

    jsonArray.push_back( histogram );

    return ;
}

void Monitor::add_hist_to_json( Graph * graphStruct, json &jsonArray, bool coverage_all ) {

   json graph = json::object();
   graph["name"] = graphStruct->name;
   graph["type"] = "graph";
   graph["title"] = graphStruct->title;
   graph["xlabel"] = graphStruct->xlabel;
   graph["ylabel"] = graphStruct->ylabel;
 
   std::vector<unsigned int> xcolumn;
   std::vector<unsigned int> ycolumn;
 
   for (auto xy : graphStruct->getObject()  ) {
       xcolumn.push_back(xy.first); 
       ycolumn.push_back(xy.second); 
    }
    
    if ( xcolumn.size() != ycolumn.size() ) {
        ERROR(__MODULEMETHOD_NAME__ << " x and y columns do not have same size. Writing empty vectors to file.");
        xcolumn.clear();
        ycolumn.clear();
    }
 
    graph["x"] = xcolumn;
    graph["y"] = ycolumn;

    jsonArray.push_back( graph );

    return ;
}

void Monitor::write_hists_to_json( HistList hist_lists, bool coverage_all ) {

  json jsonArray = json::array();

  for (auto histPtr : hist_lists.histlist ) {
	add_hist_to_json( histPtr, jsonArray, coverage_all );
  }
  for (auto histPtr : hist_lists.categoryhistlist ) {
	add_hist_to_json( histPtr, jsonArray, coverage_all );
  }
  for (auto histPtr : hist_lists.graphlist ) {
	add_hist_to_json( histPtr, jsonArray, coverage_all );
  }

  std::string full_output_path = m_outputdir + m_json_file_name;

  INFO(__MODULEMETHOD_NAME__ << " writing monitoring information to " << full_output_path );
  std::ofstream ofs( full_output_path );
 
  ofs << jsonArray.dump(4);
  ofs.close();

  return ;

}


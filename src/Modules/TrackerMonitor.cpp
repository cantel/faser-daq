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

   initialize_hists();

   m_sourceID = 1000001; // Tracker ID

   // make this configurable ...?
   m_json_file_name = "tracker_histogram_output.json";

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

  INFO( __MODULEMETHOD_NAME__ << " ... finalizing ... " );
  write_hists_to_json( m_hist_map );

  INFO(__MODULEMETHOD_NAME__ << " getState: " << this->getState());
}

void Monitor::runner() {
  INFO(__MODULEMETHOD_NAME__ << " Running...");

  const unsigned int ch = 1; // Tracker channel

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
        const EventHeader * eventHeader((EventHeader *)malloc(m_eventHeaderSize));
        EventFragmentHeader * fragmentHeader((EventFragmentHeader *)malloc(m_fragmentHeaderSize));
	bool dataOk = unpack_data( eventBuilderBinary, eventHeader, fragmentHeader );

	uint32_t eventStatus = eventHeader->status;
	fill_error_status( "h_fragmenterrors", eventStatus );
	if (!dataOk) { 
		ERROR(__MODULEMETHOD_NAME__ << " ERROR in unpacking data "); 
		m_hist_map.fillHist("h_fragmenterrors","DataUnpack");
		continue;
	}

	uint32_t fragmentStatus = fragmentHeader->status;
	fill_error_status( "h_fragmenterrors", fragmentStatus );

        uint16_t payloadSize = fragmentHeader->payload_size; 

	m_hist_map.fillHist( "h_payloadsize", payloadSize);

        timestamp_in_seconds = duration_cast<seconds>(system_clock::now().time_since_epoch());
	if ( (timestamp_in_seconds.count() - starting_time_in_seconds)%m_timeblock == 0){
	    if ( update ) { 
	    m_hist_map.fillHist("g_error_rate_per_timeblock", std::make_pair(timestamp_in_seconds.count(), m_error_rate_cnt));
	    m_error_rate_cnt = 0;
	    update = false;
	    }
 	}
	else update = true;
    }

  }

  INFO(__MODULEMETHOD_NAME__ << " Runner stopped");

}

void Monitor::initialize_hists() {

  INFO( __MODULEMETHOD_NAME__ << " ... initializing ... " );
  // TRACKER histograms

  RegularHist h_payloadsize = {"h_payloadsize","payload size [bytes]"};
  h_payloadsize.object = make_histogram(axis::regular<>(275, -0.5, 545.5, "payload size"));
  m_hist_map.addHist(h_payloadsize.name, h_payloadsize);
  //m_hist_map.reghistmap[h_payloadsize.name] = h_payloadsize;

  auto axis_fragmenterrors = axis::category<std::string>({"Ok", "Corrupted","Empty", "Missing", "BCIDMismatch", "DataUnpack"}, "error type");
  CategoryHist h_fragmenterrors = { "h_fragmenterrors", "error type" };
  h_fragmenterrors.object = make_histogram(axis_fragmenterrors);
  m_hist_map.addHist(h_fragmenterrors.name, h_fragmenterrors);
  //m_hist_map.cathistmap[h_fragmenterrors.name] = h_fragmenterrors;

  Graph g_error_rate_per_timeblock = {"g_error_rate_per_timeblock", "time [s]", "error rate" };
  m_hist_map.addHist(g_error_rate_per_timeblock.name, g_error_rate_per_timeblock);
  //m_hist_map.graphmap[g_error_rate_per_timeblock.name] = g_error_rate_per_timeblock;

  return ;

}

/*void Monitor::initialize_TLB_monitoring {

  INFO( __MODULEMETHOD_NAME__ << " ... initializing ... " );
  // TLB histograms

  auto axis_fragmenterrors = axis::category<std::string>({"Ok", "Corrupted","Empty", "Missing", "BCIDMismatch", "DataUnpack"}, "error type");
 
  h_payloadsize_rcv2.object = make_histogram(axis::regular<>(275, -0.5, 545.5, "payload size"));
  m_tlb_hist_lists.addHist(&h_payloadsize_rcv2);

  h_fragmenterrors_rcv2.object = make_histogram(axis_fragmenterrors);
  m_tlb_hist_lists.addHist(&h_fragmenterrors_rcv2);

  m_tlb_hist_lists.addHist(&g_error_rate_per_timeblock_rvc2);

  return ;

}

void Monitor::initialize_combinedFragment_monitoring( ) {

  INFO( __MODULEMETHOD_NAME__ << " ... initializing ... " );
  // combined fragments histograms

  h_timedelay_rcv1_rcv2.object = make_histogram(axis::regular<>(100, -500., 500., "time [mus]"));
  m_combinedfragments_hist_lists.addHist(&h_timedelay_rcv1_rcv2);
  
    return ;

}*/

bool Monitor::unpack_data( daqling::utilities::Binary eventBuilderBinary, const EventHeader *& eventHeader, EventFragmentHeader *& fragmentHeader ) {

	bool dataOk =false;
	
        eventHeader = static_cast< const EventHeader *>(eventBuilderBinary.data());
        uint16_t numChannels = eventHeader->num_fragments;
        uint32_t totalDataPacketSize = eventBuilderBinary.size();

        /*if ( m_chNo > numChannels ) {
		ERROR( __MODULEMETHOD_NAME__ <<  " Insufficient number of fragments received: Only "<<numChannels<< " fragment(s) found.");
		return dataOk = false;
	}*/

	uint32_t accumulatedPayloadSize = 0;
	uint8_t cnt = 0;

        for ( unsigned int chidx=0; chidx<numChannels; ++chidx){
	
	    cnt++;	
            EventFragmentHeader * currentChannelFragmentHeader((EventFragmentHeader *)malloc(m_fragmentHeaderSize));
	    memcpy( currentChannelFragmentHeader, static_cast<const char*>(eventBuilderBinary.data())+m_eventHeaderSize+m_fragmentHeaderSize*(chidx)+accumulatedPayloadSize, m_fragmentHeaderSize);

	    uint32_t source_id = currentChannelFragmentHeader->source_id;

            accumulatedPayloadSize += currentChannelFragmentHeader->payload_size; 

            if ( source_id == m_sourceID ) {
		fragmentHeader = currentChannelFragmentHeader;
		dataOk = true; // found channel
		break;
	    }
	}


	if ( dataOk ) {
	    // sanity check
            auto totalCalculatedSize = m_eventHeaderSize+m_fragmentHeaderSize*cnt+accumulatedPayloadSize ;
	    if ( totalCalculatedSize > totalDataPacketSize ) {
	    	ERROR(__MODULEMETHOD_NAME__ <<  "total byte size unpacked, "
	    	              << totalCalculatedSize 
                                  <<", larger than the total data packet size, "
                                  <<totalDataPacketSize
                                  <<". FIX ME." );
	    	return dataOk = false;
	    }
	}
	else {
	    	ERROR(__MODULEMETHOD_NAME__ <<  " fragment with correct source ID not found.");
	}

	return dataOk;

}

void Monitor::fill_error_status(CategoryHist &hist,  uint32_t fragmentStatus ) {

    if ( fragmentStatus == 0 ) hist.object( "Ok");
    else {
        m_error_rate_cnt+=1;
        if ( fragmentStatus & CorruptedFragment ) hist.object( "Corrupted");
        if ( fragmentStatus & EmptyFragment ) hist.object( "Empty");
        if ( fragmentStatus & MissingFragment ) hist.object( "Missing");
        if ( fragmentStatus & BCIDMismatch ) hist.object( "BCIDMismatch");
    }
    
    return ;

}

void Monitor::fill_error_status(std::string hist_name, uint32_t fragmentStatus ) {

    if ( fragmentStatus == 0 ) m_hist_map.fillHist(hist_name, "Ok");
    else {
        m_error_rate_cnt+=1;
        if ( fragmentStatus & CorruptedFragment ) m_hist_map.fillHist( hist_name, "Corrupted");
        if ( fragmentStatus & EmptyFragment ) m_hist_map.fillHist( hist_name, "Empty");
        if ( fragmentStatus & MissingFragment ) m_hist_map.fillHist(hist_name,  "Missing");
        if ( fragmentStatus & BCIDMismatch ) m_hist_map.fillHist( hist_name, "BCIDMismatch");
    }
    
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
void Monitor::add_hist_to_json( T histStruct, json &jsonArray, bool coverage_all ) {


   auto hist = histStruct.getObject();
   
   float binwidth =  hist.axis().bin(1).upper() - hist.axis().bin(2).lower();
 
   json histogram = json::object();
   histogram["name"] = histStruct.name;
   histogram["type"] = "regular";
   histogram["title"] = histStruct.title;
   histogram["xlabel"] = histStruct.xlabel;
   histogram["ylabel"] = histStruct.ylabel;
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

void Monitor::add_hist_to_json( CategoryHist histStruct, json &jsonArray, bool coverage_all ) {

   auto hist = histStruct.getObject();
  
   json histogram = json::object();
   histogram["name"] = histStruct.name;
   histogram["type"] = "category";
   histogram["title"] = histStruct.title;
   histogram["xlabel"] = histStruct.xlabel;
   histogram["ylabel"] = histStruct.ylabel;
 
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

void Monitor::add_hist_to_json( Graph graphStruct, json &jsonArray, bool coverage_all ) {

   json graph = json::object();
   graph["name"] = graphStruct.name;
   graph["type"] = "graph";
   graph["title"] = graphStruct.title;
   graph["xlabel"] = graphStruct.xlabel;
   graph["ylabel"] = graphStruct.ylabel;
 
   std::vector<unsigned int> xcolumn;
   std::vector<unsigned int> ycolumn;
 
   for (auto xy : graphStruct.getObject()  ) {
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

  for (auto hist : hist_lists.histlist ) {
	add_hist_to_json( *hist, jsonArray, coverage_all );
  }
  for (auto hist : hist_lists.categoryhistlist ) {
	add_hist_to_json( *hist, jsonArray, coverage_all );
  }
  for (auto hist : hist_lists.graphlist ) {
	add_hist_to_json( *hist, jsonArray, coverage_all );
  }

  std::string full_output_path = m_outputdir + m_json_file_name;

  INFO(__MODULEMETHOD_NAME__ << " writing monitoring information to " << full_output_path );
  std::ofstream ofs( full_output_path );
 
  ofs << jsonArray.dump(4);
  ofs.close();

  return ;

}

void Monitor::write_hists_to_json( HistMaps hist_lists, bool coverage_all ) {

  json jsonArray = json::array();

  for (auto hist : hist_lists.reghistmap ) {
	add_hist_to_json( hist.second, jsonArray, coverage_all );
  }
  for (auto hist : hist_lists.cathistmap ) {
	add_hist_to_json( hist.second, jsonArray, coverage_all );
  }
  for (auto hist : hist_lists.graphmap ) {
	add_hist_to_json( hist.second, jsonArray, coverage_all );
  }

  std::string full_output_path = m_outputdir + m_json_file_name;

  INFO(__MODULEMETHOD_NAME__ << " writing monitoring information to " << full_output_path );
  std::ofstream ofs( full_output_path );
 
  ofs << jsonArray.dump(4);
  ofs.close();

  return ;

}


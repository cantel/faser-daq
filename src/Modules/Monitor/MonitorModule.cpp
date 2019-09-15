/// \cond
#include <chrono>
#include <map>
#include <boost/format.hpp>
#include <boost/histogram/ostream.hpp> // write histogram straight to ostream
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream
/// \endcond

#include "MonitorModule.hpp"
#include "Core/Statistics.hpp"

using namespace boost::histogram;
using namespace std::chrono_literals;
using namespace std::chrono;

MonitorModule::MonitorModule() { 
   INFO("");
 }

MonitorModule::~MonitorModule() { 
  INFO("With config: " << m_config.dump() << " getState: " << this->getState());
}

void MonitorModule::start() {
  DAQProcess::start();
  INFO("getState: " << this->getState());

  initialize_hists();
  register_metrics();

}

void MonitorModule::stop() {
  DAQProcess::stop();

  INFO("... finalizing ...");
  write_hists_to_json( m_hist_map );

  INFO("getState: " << this->getState());
}

void MonitorModule::runner() {
  INFO("Running...");

  while (m_run) {
	//do nothing
  }

  INFO("Runner stopped");

  return;

}

void MonitorModule::initialize_hists() {

  INFO("... initializing ... " );
  return;

}

void MonitorModule::register_metrics() {

 INFO("... registering metrics in base MonitorModule class ... " );

}

uint16_t MonitorModule::unpack_data( daqling::utilities::Binary &eventBuilderBinary, const EventHeader *& eventHeader, EventFragmentHeader *& fragmentHeader ) {

  uint16_t dataStatus=0;
  bool foundSourceID(false);

  uint16_t fragmentCnt = eventHeader->fragment_count;
  uint32_t totalDataPacketSize = eventBuilderBinary.size();

  uint32_t accumulatedPayloadSize = 0;
  uint8_t cnt = 0;

  // Claire: to do: need to incorporate markers.
  for ( unsigned int frgidx=0; frgidx<fragmentCnt; ++frgidx){
	
	  cnt++;	
    EventFragmentHeader * currentChannelFragmentHeader((EventFragmentHeader *)malloc(m_fragmentHeaderSize));
	  memcpy( currentChannelFragmentHeader, static_cast<const char*>(eventBuilderBinary.data())+m_eventHeaderSize+m_fragmentHeaderSize*(frgidx)+accumulatedPayloadSize, m_fragmentHeaderSize);

    if ( m_fragmentHeaderSize != currentChannelFragmentHeader->header_size )  ERROR("fragment header gives wrong size!");

     uint32_t source_id = currentChannelFragmentHeader->source_id;

     if ( source_id == m_sourceID ) {
		   fragmentHeader = currentChannelFragmentHeader;
		   foundSourceID = true;
		   break;
     }
     accumulatedPayloadSize += currentChannelFragmentHeader->payload_size; 
  }


	if ( foundSourceID ) {
	  // sanity check
    auto totalCalculatedSize = m_eventHeaderSize+m_fragmentHeaderSize*cnt+accumulatedPayloadSize ;
	  if ( totalCalculatedSize > totalDataPacketSize ) {
	  	ERROR("total byte size unpacked, "
	  	                            << totalCalculatedSize 
                                  <<", larger than the total data packet size, "
                                  <<totalDataPacketSize
                                  <<". FIX ME." );
	  	return dataStatus |= CorruptedFragment; //To review: is this the right error type for this?
	  }
	}
	else {
	    	ERROR("no correct fragment source ID found.");
		dataStatus |= MissingFragment;
	}

	return dataStatus;
}

void MonitorModule::fill_error_status(CategoryHist &hist,  uint32_t fragmentStatus ) {

  if ( fragmentStatus == 0 ) hist.object( "Ok");
  else {
     if ( fragmentStatus  &  UnclassifiedError ) hist.object( "Unclassified");
     if ( fragmentStatus  &  BCIDMismatch ) hist.object( "BCIDMismatch");
     if ( fragmentStatus  &  TagMismatch ) hist.object( "TagMismatch");
     if ( fragmentStatus  &  Timeout ) hist.object( "Timeout");
     if ( fragmentStatus  &  Overflow ) hist.object( "Overflow");
     if ( fragmentStatus  &  CorruptedFragment ) hist.object( "Corrupted");
     if ( fragmentStatus  &  DummyFragment ) hist.object( "Dummy");
     if ( fragmentStatus  &  MissingFragment ) hist.object( "Missing");
     if ( fragmentStatus  &  EmptyFragment ) hist.object( "Empty");
     if ( fragmentStatus  &  DuplicateFragment ) hist.object( "Duplicate");
  }
  
  return ;

}

void MonitorModule::fill_error_status(std::string hist_name, uint32_t fragmentStatus ) {

  if ( fragmentStatus == 0 ) m_hist_map.fillHist(hist_name, "Ok");
  else {
     if ( fragmentStatus  &  UnclassifiedError ) m_hist_map.fillHist( hist_name, "Unclassified");
     if ( fragmentStatus  &  BCIDMismatch ) m_hist_map.fillHist( hist_name, "BCIDMismatch");
     if ( fragmentStatus  &  TagMismatch ) m_hist_map.fillHist( hist_name, "TagMismatch");
     if ( fragmentStatus  &  Timeout ) m_hist_map.fillHist( hist_name, "Timeout");
     if ( fragmentStatus  &  Overflow ) m_hist_map.fillHist( hist_name, "Overflow");
     if ( fragmentStatus  &  CorruptedFragment ) m_hist_map.fillHist( hist_name, "Corrupted");
     if ( fragmentStatus  &  DummyFragment ) m_hist_map.fillHist( hist_name, "Dummy");
     if ( fragmentStatus  &  MissingFragment ) m_hist_map.fillHist( hist_name, "Missing");
     if ( fragmentStatus  &  EmptyFragment ) m_hist_map.fillHist( hist_name, "Empty");
     if ( fragmentStatus  &  DuplicateFragment ) m_hist_map.fillHist( hist_name, "Duplicate");
  }
  
  return ;

}

void MonitorModule::fill_error_status( uint32_t fragmentStatus ) {

  std::cout<<"fragmentStatus = "<<fragmentStatus<<std::endl;
  if ( fragmentStatus == 0 ) m_metric_error_ok += 1;
  else {
      if ( fragmentStatus  &  UnclassifiedError ) m_metric_error_unclassified += 1;
      if ( fragmentStatus  &  BCIDMismatch ) m_metric_error_bcidmismatch += 1;
      if ( fragmentStatus  &  TagMismatch ) m_metric_error_tagmismatch += 1;
      if ( fragmentStatus  &  Timeout ) m_metric_error_timeout += 1;
      if ( fragmentStatus  &  Overflow ) m_metric_error_overflow += 1;
      if ( fragmentStatus  &  CorruptedFragment ) m_metric_error_corrupted += 1;
      if ( fragmentStatus  &  DummyFragment ) m_metric_error_dummy += 1;
      if ( fragmentStatus  &  MissingFragment ) m_metric_error_missing += 1;
      if ( fragmentStatus  &  EmptyFragment ) m_metric_error_empty += 1;
      if ( fragmentStatus  &  DuplicateFragment ) m_metric_error_duplicate += 1;
  }
  
  return ;

}

template <typename T>
void MonitorModule::flush_hist( T histStruct, bool coverage_all ) {

  INFO("flushing hist info for "<< histStruct.name);

  std::ostringstream os;
  auto hist = histStruct.getObject();
  auto this_axis = hist.axis();
  auto thiscoverage = coverage::all;
  if ( !coverage_all ) thiscoverage = coverage::inner;

  for (auto x : indexed(hist, thiscoverage) ) { 
     os << boost::format("bin %2i bin value %s : %i\n") % x.index() % this_axis.value(x.index()) % *x;
  }
}

void MonitorModule::flush_hist( CategoryHist histStruct, bool coverage_all ) {

  INFO("flushing hist info for "<<histStruct.name);
  INFO("hist axis is of type category; hist has no under/overflow bins. ");
  
  std::ostringstream os;
  auto hist = histStruct.getObject();
  auto this_axis = hist.axis();
  
   for (auto x : indexed(hist, coverage::inner) ) { // no under/overflow bins for category axis. 
      os << boost::format("bin %2i bin value %s : %i\n") % x.index() % this_axis.value(x.index()) % *x;
   }
  
  std::cout << os.str() << std::flush;
  
  return ;

}

void MonitorModule::flush_hist( Graph histStruct, bool coverage_all ) {

  INFO("flushing hist info for "<<histStruct.name);
  
  std::ostringstream os;
  auto hist = histStruct.getObject();
  
   for (auto xy : histStruct.getObject()  ) {
      os << boost::format("x:  %2f y: %2f \n") % xy.first % xy.second;
   }
  
  std::cout << os.str() << std::flush;
  
  return ;

}

template <typename T>
void MonitorModule::write_hist_to_file( T histStruct, std::string dir, bool coverage_all ) {

  auto hist = histStruct.getObject(); 
  std::string file_name = dir + "histoutput_"+histStruct.name+".txt";

  INFO("writing hist info  to file ... "<< file_name);

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
void MonitorModule::add_hist_to_json( T histStruct, json &jsonArray, bool coverage_all ) {


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
      ERROR("x and y columns do not have same size. Writing empty vectors to file.");
      xcolumn.clear();
      ycolumn.clear();
  }

  histogram["x"] = xcolumn;
  histogram["y"] = ycolumn;

  jsonArray.push_back( histogram );

  return ;
}

void MonitorModule::add_hist_to_json( CategoryHist histStruct, json &jsonArray, bool coverage_all ) {

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
       ERROR("x and y columns do not have same size. Writing empty vectors to file.");
       xcolumn.clear();
       ycolumn.clear();
  }
 
  histogram["x"] = xcolumn;
  histogram["y"] = ycolumn;

  jsonArray.push_back( histogram );

  return ;
}

void MonitorModule::add_hist_to_json( Graph graphStruct, json &jsonArray, bool coverage_all ) {

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
      ERROR("x and y columns do not have same size. Writing empty vectors to file.");
      xcolumn.clear();
      ycolumn.clear();
  }
 
  graph["x"] = xcolumn;
  graph["y"] = ycolumn;

  jsonArray.push_back( graph );

  return ;
}

void MonitorModule::write_hists_to_json( HistList hist_lists, bool coverage_all ) {

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

  INFO("writing monitoring information to " << full_output_path );
  std::ofstream ofs( full_output_path );
 
  ofs << jsonArray.dump(4);
  ofs.close();

  return ;

}

void MonitorModule::write_hists_to_json( HistMaps hist_lists, bool coverage_all ) {

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

  INFO("writing monitoring information to " << full_output_path );
  std::ofstream ofs( full_output_path );
 
  ofs << jsonArray.dump(4);
  ofs.close();

  return ;

}


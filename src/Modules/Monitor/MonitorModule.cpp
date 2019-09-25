/// \cond
#include <chrono>
#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream
/// \endcond

#include "MonitorModule.hpp"
#include "Core/Statistics.hpp"

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

   m_histogramming_on = false;
   setupHistogramManager();

  register_metrics();
  register_hists();
}

void MonitorModule::stop() {
  DAQProcess::stop();

  INFO("... finalizing ...");

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

void MonitorModule::register_metrics() {

 INFO("... registering metrics in base MonitorModule class ... " );

}

void MonitorModule::register_hists() {

 INFO(" ... registering histograms in base Monitor class ... " );

}

uint16_t MonitorModule::unpack_data( daqling::utilities::Binary &eventBuilderBinary, EventHeader *& eventHeader, EventFragmentHeader *& fragmentHeader ) {

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

void MonitorModule::setupHistogramManager() {

  INFO("Setting up HistogramManager.");

  m_histogrammanager = std::make_unique<HistogramManager>(m_connections.getStatSocket(),500.);
  
  m_histogrammanager->start();
 
  m_histogramming_on = true;

  if ( m_stats_on ) {
    INFO("Statistics is on. Will publish histograms via the stats connection.");
    m_histogrammanager->setZMQpublishing(true);

  }
  else {
    INFO("Statistics is off. Will not be publishing histograms.");
  }

  return ;

}

void MonitorModule::fill_error_status_to_metric( uint32_t fragmentStatus ) {

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

void MonitorModule::fill_error_status_to_histogram( uint32_t fragmentStatus, std::string hist_name ) {

  if ( fragmentStatus == 0 ) m_histogrammanager->fill(hist_name, "Ok");
  else {
      if ( fragmentStatus  &  UnclassifiedError ) m_histogrammanager->fill(hist_name, "Unclassified");
      if ( fragmentStatus  &  BCIDMismatch ) m_histogrammanager->fill(hist_name, "BCIDMismatch");
      if ( fragmentStatus  &  TagMismatch ) m_histogrammanager->fill(hist_name, "TagMismatch");
      if ( fragmentStatus  &  Timeout ) m_histogrammanager->fill(hist_name, "Timeout");
      if ( fragmentStatus  &  Overflow ) m_histogrammanager->fill(hist_name, "Overflow");
      if ( fragmentStatus  &  CorruptedFragment ) m_histogrammanager->fill(hist_name, "CorruptedFragment");
      if ( fragmentStatus  &  DummyFragment ) m_histogrammanager->fill(hist_name, "Dummy");
      if ( fragmentStatus  &  MissingFragment ) m_histogrammanager->fill(hist_name, "Missing");
      if ( fragmentStatus  &  EmptyFragment ) m_histogrammanager->fill(hist_name, "Empty");
      if ( fragmentStatus  &  DuplicateFragment ) m_histogrammanager->fill(hist_name, "Duplicate");
  }
  
  return ;

}

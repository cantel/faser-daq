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
   m_histogramming_on = false;
 }

MonitorModule::~MonitorModule() { 

  delete m_eventHeader;
  delete m_fragmentHeader;
  delete m_rawFragment;

  INFO("With config: " << m_config.dump() << " getState: " << this->getState());
}

void MonitorModule::start() {
  DAQProcess::start();
  INFO("getState: " << this->getState());

  setupHistogramManager();

  register_metrics();
  register_hists();

  m_histogrammanager->start();

}

void MonitorModule::stop() {
  DAQProcess::stop();

  INFO("... finalizing ...");

  INFO("getState: " << this->getState());
}

void MonitorModule::runner() {
  INFO("Running...");

  m_event_header_unpacked = false;
  m_fragment_header_unpacked = false;
  m_raw_fragment_unpacked = false;
  bool noData(true);
  daqling::utilities::Binary eventBuilderBinary;

  while (m_run) {

      if ( !m_connections.get(1, eventBuilderBinary)){
          if ( !noData ) std::this_thread::sleep_for(10ms);
          noData=true;
          continue;
      }
      noData=false;

      monitor(eventBuilderBinary);

      m_event_header_unpacked = false;
      m_fragment_header_unpacked = false;
      m_raw_fragment_unpacked = false;
  }


  INFO("Runner stopped");

  return;

}

void MonitorModule::monitor(daqling::utilities::Binary&) {
}

void MonitorModule::register_metrics() {

 INFO("... registering metrics in base MonitorModule class ... " );

}

void MonitorModule::register_hists() {

 INFO(" ... registering histograms in base Monitor class ... " );

}

uint16_t MonitorModule::unpack_event_header( daqling::utilities::Binary &eventBuilderBinary ) {

  uint16_t dataStatus(0);

  m_eventHeader = static_cast<EventHeader *>(eventBuilderBinary.data());
  m_event_header_unpacked = true;
  if (m_eventHeader->marker != EventMarker) {
    ERROR("Corrupted event header. Wrong event marker returned.");
    return dataStatus |= CorruptedFragment;
  }

  return  dataStatus;

}

uint16_t MonitorModule::unpack_fragment_header( daqling::utilities::Binary &eventBuilderBinary ) {

  uint16_t dataStatus=0;
  bool foundSourceID(false);

  if (!m_event_header_unpacked) {
    m_eventHeader = static_cast<EventHeader *>(eventBuilderBinary.data());
     m_event_header_unpacked = true;
    if (m_eventHeader->marker != EventMarker) {
      ERROR("Corrupted event header. Wrong event marker returned.");
      return dataStatus |= CorruptedFragment;
     }
  }

  uint16_t fragmentCnt = m_eventHeader->fragment_count;
  uint32_t totalDataPacketSize = eventBuilderBinary.size();

  uint32_t accumulatedPayloadSize = 0;
  uint8_t cnt = 0;

  EventFragmentHeader * currentChannelFragmentHeader = new EventFragmentHeader;

  for ( unsigned int frgidx=0; frgidx<fragmentCnt; ++frgidx){
	
    cnt++;	
    memcpy( currentChannelFragmentHeader, static_cast<const char*>(eventBuilderBinary.data())+m_eventHeaderSize+m_fragmentHeaderSize*(frgidx)+accumulatedPayloadSize, m_fragmentHeaderSize);

    if ( m_fragmentHeaderSize != currentChannelFragmentHeader->header_size )  ERROR("fragment header gives wrong size!");

     uint32_t source_id = currentChannelFragmentHeader->source_id;

     if ( source_id == m_sourceID ) {
		   foundSourceID = true;
		   memcpy(m_fragmentHeader, currentChannelFragmentHeader, m_fragmentHeaderSize);
                   m_fragment_header_unpacked = true;
                   if (m_fragmentHeader->marker != FragmentMarker){
      		     ERROR("Corrupted fragment. Wrong fragment marker returned.");
                     return dataStatus |= CorruptedFragment;
                   }
		   break;
     }
     accumulatedPayloadSize += currentChannelFragmentHeader->payload_size; 
  }
  delete currentChannelFragmentHeader;


  if ( foundSourceID ) {
    // sanity check
    auto totalCalculatedSize = m_eventHeaderSize+m_fragmentHeaderSize*cnt+accumulatedPayloadSize ;
    if ( totalCalculatedSize > totalDataPacketSize ) {
    	ERROR("total byte size unpacked, "
    	                            << totalCalculatedSize 
                            <<", larger than the total data packet size, "
                            <<totalDataPacketSize
                            <<". FIX ME." );
    	return dataStatus |= CorruptedFragment;
    }
  }
  else {
      	ERROR("no correct fragment source ID found.");
  	dataStatus |= MissingFragment;
  }
  return dataStatus;
}

uint16_t MonitorModule::unpack_full_fragment( daqling::utilities::Binary &eventBuilderBinary ) {

  uint16_t dataStatus=0;
  bool foundSourceID(false);

  if (!m_event_header_unpacked) {
     m_eventHeader = static_cast<EventHeader *>(eventBuilderBinary.data());
     m_event_header_unpacked = true;
  }

  uint16_t fragmentCnt = m_eventHeader->fragment_count;
  uint32_t totalDataPacketSize = eventBuilderBinary.size();

  uint32_t accumulatedPayloadSize = 0;
  uint8_t cnt = 0;

  EventFragmentHeader * currentChannelFragmentHeader = new EventFragmentHeader;

  for ( unsigned int frgidx=0; frgidx<fragmentCnt; ++frgidx){
	
    cnt++;	
    memcpy( currentChannelFragmentHeader, static_cast<const char*>(eventBuilderBinary.data())+m_eventHeaderSize+m_fragmentHeaderSize*(frgidx)+accumulatedPayloadSize, m_fragmentHeaderSize);

    if ( m_fragmentHeaderSize != currentChannelFragmentHeader->header_size )  ERROR("fragment header gives wrong size!");

     uint32_t source_id = currentChannelFragmentHeader->source_id;

     if ( source_id == m_sourceID ) {
		   foundSourceID = true;
                   memcpy(m_rawFragment, static_cast<const char*>(eventBuilderBinary.data())+m_eventHeaderSize+m_fragmentHeaderSize*(frgidx)+accumulatedPayloadSize+m_fragmentHeaderSize, m_rawFragmentSize );
		   memcpy(m_fragmentHeader, currentChannelFragmentHeader, m_fragmentHeaderSize);
                   m_fragment_header_unpacked = true;
                   m_raw_fragment_unpacked = true;
                   if (m_fragmentHeader->marker != FragmentMarker){
      		     ERROR("Corrupted fragment. Wrong fragment marker returned.");
                     return dataStatus |= CorruptedFragment;
                   }
		   break;
     }
     accumulatedPayloadSize += currentChannelFragmentHeader->payload_size; 
  }
  delete currentChannelFragmentHeader;


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

  m_histogrammanager = std::make_unique<HistogramManager>(m_connections.getStatSocket());

  m_histogramming_on = true;

  auto statsURI = m_config.getConfig()["settings"]["stats_uri"];
  if (statsURI != "" && statsURI != nullptr) {
    INFO("Stats uri provided. Will publish histograms via the stats connection.");
    m_histogrammanager->setZMQpublishing(true);

  }
  else {
    INFO("No stats uri given. Will not be publishing histograms.");
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

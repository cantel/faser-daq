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

   auto cfg = m_config.getSettings();
   auto cfg_sourceID = m_config.getConfig()["settings"]["fragmentID"];
   std::cout<<"cf_sourceID = "<<cfg_sourceID<<std::endl;
   if (cfg_sourceID!="" && cfg_sourceID!=nullptr)
      m_sourceID = cfg_sourceID;
   else m_sourceID=0;
   auto cfg_tag = m_config.getConfig()["settings"]["eventTag"];
   std::cout<<"cf_tag = "<<cfg_tag<<std::endl;
   if (cfg_tag!="" && cfg_tag!=nullptr) m_eventTag = cfg_tag;
   else {
     WARNING("No event tag configured. Defaulting to PhysicsTag.");
     m_eventTag=0; //default to Physics. ... should it though?
   }
 
   if (m_eventTag>TLBMonitoringTag) { // not needed once have module-level schema validation.
     ERROR("Configured tag does not exist!");
     m_status = STATUS_ERROR;
   }

 }

MonitorModule::~MonitorModule() { 

  if (m_histogramming_on) m_histogrammanager->stop();
  delete m_event;

  INFO("With config: " << m_config.dump() << " getState: " << this->getState());
}

void MonitorModule::configure() {
  INFO("Configuring...");

  FaserProcess::configure();
  register_metrics();

  m_histogramming_on = false;
  setupHistogramManager();
  register_hists();

  return;
}

void MonitorModule::start(int run_num) {
  FaserProcess::start(run_num);
  INFO("getState: " << this->getState());

  if ( m_histogramming_on ) m_histogrammanager->start();

}

void MonitorModule::stop() {
  FaserProcess::stop();

  INFO("... finalizing ...");
  INFO("getState: " << this->getState());
}

void MonitorModule::runner() {
  INFO("Running...");

  m_event_header_unpacked = false;
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
  }


  INFO("Runner stopped");

  return;

}

void MonitorModule::monitor(daqling::utilities::Binary&) {
}

void MonitorModule::register_metrics() {

 INFO("... registering metrics in base MonitorModule class ... " );

}

void MonitorModule::register_error_metrics( std::string module_short_name) {

   m_metric_payload=0;
   m_metric_error_ok=0;
   m_metric_error_unclassified=0;
   m_metric_error_bcidmismatch=0;
   m_metric_error_tagmismatch=0;
   m_metric_error_timeout=0;
   m_metric_error_overflow=0;
   m_metric_error_corrupted=0;
   m_metric_error_dummy=0;
   m_metric_error_missing=0;
   m_metric_error_empty=0;
   m_metric_error_duplicate=0;
   m_metric_error_unpack=0;

  if ( m_stats_on ) {
    INFO("... registering error metrics ... " );
    m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_ok, module_short_name+"_error_ok", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_unclassified, module_short_name+"_error_unclassified", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_bcidmismatch, module_short_name+"_error_bcidmismatch", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_tagmismatch, module_short_name+"_error_tagmismatch", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_timeout, module_short_name+"_error_timeout", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_overflow, module_short_name+"_error_overflow", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_corrupted, module_short_name+"_error_corrupted", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_dummy, module_short_name+"_error_dummy", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_unpack, module_short_name+"_error_unpack", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_missing, module_short_name+"_error_missing", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_empty, module_short_name+"_error_empty", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);
    m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_duplicate, module_short_name+"_error_duplicate", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);
  }
  return;

}

void MonitorModule::register_hists() {

 INFO(" ... No histograms to register. Will not start histogram service. " );
 m_histogramming_on = false;

}

uint16_t MonitorModule::unpack_event_header( daqling::utilities::Binary &eventBuilderBinary ) {

  uint16_t dataStatus(0);
  try {
    if (m_event) delete m_event;
    m_event = new EventFull(eventBuilderBinary); //BP note that this unpacks full event
  } catch (const std::runtime_error& e) {
    ERROR(e.what());
    return dataStatus |= CorruptedFragment;
  }
  m_event_header_unpacked = true;
  
  return  dataStatus;

}

uint16_t MonitorModule::unpack_fragment_header( daqling::utilities::Binary &eventBuilderBinary, uint32_t sourceID ) {

  uint16_t dataStatus=0;

  if (!m_event_header_unpacked) {
    dataStatus=unpack_event_header(eventBuilderBinary);
    if (dataStatus) return dataStatus;
  }

  m_fragment=m_event->find_fragment(sourceID);
  if (m_fragment==0) {
    ERROR("No correct fragment source ID found.");
    dataStatus |= MissingFragment;
  }
  return dataStatus;
}

uint16_t MonitorModule::unpack_fragment_header( daqling::utilities::Binary &eventBuilderBinary ) {
  auto dataStatus = unpack_fragment_header(eventBuilderBinary, m_sourceID);
  return dataStatus;
}

uint16_t MonitorModule::unpack_full_fragment( daqling::utilities::Binary &eventBuilderBinary, uint32_t sourceID ) {

  uint16_t dataStatus=0;

  dataStatus=unpack_fragment_header(eventBuilderBinary, sourceID);
  if (dataStatus) return dataStatus;

  switch (m_eventTag) {
    case PhysicsTag:
      m_rawFragment=m_fragment->payload<const RawFragment*>();
      break;
    case CalibrationTag:
      m_rawFragment=m_fragment->payload<const RawFragment*>();
      break;
    case MonitoringTag:
      m_monitoringFragment=m_fragment->payload<const MonitoringFragment*>();
      break;
    case TLBMonitoringTag:
      m_monitoringFragment=m_fragment->payload<const MonitoringFragment*>(); //to be changed once subdetector specific formats defined.
      break;
    default:
      ERROR("Specified tag not found.");
      dataStatus |= UnclassifiedError;
  }
  
  return dataStatus;
}

uint16_t MonitorModule::unpack_full_fragment( daqling::utilities::Binary &eventBuilderBinary) {
  auto dataStatus = unpack_full_fragment(eventBuilderBinary, m_sourceID);
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

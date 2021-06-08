/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
/// \cond
#include <chrono>
#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream
/// \endcond

#include "MonitorBaseModule.hpp"
#include "Core/Statistics.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;
using namespace MonitorBase;
MonitorBaseModule::MonitorBaseModule(const std::string& n):FaserProcess(n) { 
   INFO("");

   auto cfg = m_config.getModuleSettings(getName());
   auto cfg_sourceID = cfg["fragmentID"];
   if (cfg_sourceID!="" && cfg_sourceID!=nullptr)
      m_sourceID = cfg_sourceID;
   else m_sourceID=0;

 }

MonitorBaseModule::~MonitorBaseModule() { 

  delete m_event;
  if (m_trackerdataFragment) delete m_trackerdataFragment;

  INFO("With config: " << m_config.dump());
}

void MonitorBaseModule::configure() {
  INFO("Configuring...");

  FaserProcess::configure();
  register_metrics();

  m_histogramming_on = false;
  setupHistogramManager();
  register_hists();

  return;
}

void MonitorBaseModule::start(unsigned int run_num) {
  FaserProcess::start(run_num);

  if ( m_histogramming_on ) m_histogrammanager->start();

}

void MonitorBaseModule::stop() {
  FaserProcess::stop();

  INFO("... finalizing ...");
  if (m_histogramming_on) m_histogrammanager->stop();

}

void MonitorBaseModule::runner() noexcept {
  INFO("Running...");

  m_event_header_unpacked = false;
  DataFragment<daqling::utilities::Binary> eventBuilderBinary;

  while (m_run) {

      if ( !m_connections.receive(0, eventBuilderBinary)){
          std::this_thread::sleep_for(10ms);
          continue;
      }

      monitor(eventBuilderBinary);

      m_event_header_unpacked = false;
  }


  INFO("Runner stopped");

  return;

}

void MonitorBaseModule::monitor(DataFragment<daqling::utilities::Binary>&) {
}

void MonitorBaseModule::register_metrics() {

 INFO("... registering metrics in base MonitorBaseModule class ... " );

}

void MonitorBaseModule::register_error_metrics() {

   m_metric_payload=0;
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
    m_statistics->registerMetric(&m_metric_error_unclassified, "error_unclassified", daqling::core::metrics::ACCUMULATE);
    m_statistics->registerMetric(&m_metric_error_bcidmismatch, "error_bcidmismatch", daqling::core::metrics::ACCUMULATE);
    m_statistics->registerMetric(&m_metric_error_tagmismatch, "error_tagmismatch", daqling::core::metrics::ACCUMULATE);
    m_statistics->registerMetric(&m_metric_error_timeout, "error_timeout", daqling::core::metrics::ACCUMULATE);
    m_statistics->registerMetric(&m_metric_error_overflow, "error_overflow", daqling::core::metrics::ACCUMULATE);
    m_statistics->registerMetric(&m_metric_error_corrupted, "error_corrupted", daqling::core::metrics::ACCUMULATE);
    m_statistics->registerMetric(&m_metric_error_dummy, "error_dummy", daqling::core::metrics::ACCUMULATE);
    m_statistics->registerMetric(&m_metric_error_unpack, "error_unpack", daqling::core::metrics::ACCUMULATE);
    m_statistics->registerMetric(&m_metric_error_missing, "error_missing", daqling::core::metrics::ACCUMULATE);
    m_statistics->registerMetric(&m_metric_error_empty, "error_empty", daqling::core::metrics::ACCUMULATE);
    m_statistics->registerMetric(&m_metric_error_duplicate, "error_duplicate", daqling::core::metrics::ACCUMULATE);
  }
  return;

}

void MonitorBaseModule::register_hists() {

 INFO(" ... No histograms to register. Will not start histogram service. " );
 m_histogramming_on = false;

}

uint16_t MonitorBaseModule::unpack_event_header( DataFragment<daqling::utilities::Binary> &eventBuilderBinary ) {

  uint16_t dataStatus(0);
  try {
    if (m_event) delete m_event;
    m_event = new EventFull(eventBuilderBinary.data<const uint8_t*>(),eventBuilderBinary.size()); //BP note that this unpacks full event
  } catch (const std::runtime_error& e) {
    ERROR(e.what());
    return dataStatus |= CorruptedFragment;
  }
  m_event_header_unpacked = true;
  m_eventTag = m_event->event_tag();
  
  return  dataStatus;

}

uint16_t MonitorBaseModule::unpack_fragment_header( DataFragment<daqling::utilities::Binary> &eventBuilderBinary, uint32_t sourceID ) {

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

uint16_t MonitorBaseModule::unpack_fragment_header( DataFragment<daqling::utilities::Binary> &eventBuilderBinary ) {
  auto dataStatus = unpack_fragment_header(eventBuilderBinary, m_sourceID);
  return dataStatus;
}

uint16_t MonitorBaseModule::unpack_full_fragment( DataFragment<daqling::utilities::Binary> &eventBuilderBinary, uint32_t sourceID ) {

  uint16_t dataStatus=0;

  dataStatus=unpack_fragment_header(eventBuilderBinary, sourceID);
  if (dataStatus) return dataStatus;

  switch (m_eventTag) {
    case PhysicsTag:{
      switch (sourceID&0xFFFFFF00) {
        case TriggerSourceID:
          m_tlbdataFragment = std::make_unique<TLBDataFragment>(TLBDataFragment(m_fragment->payload<const uint32_t*>(), m_fragment->payload_size()));
          DEBUG("unpacking TLB data fragment.");
          break;
        case PMTSourceID:
          m_pmtdataFragment = std::make_unique<DigitizerDataFragment>(DigitizerDataFragment(m_fragment->payload<const uint32_t*>(), m_fragment->payload_size()));
          DEBUG("unpacking PMT data fragment.");
          break;
        case TrackerSourceID:
          if (m_trackerdataFragment) delete m_trackerdataFragment;
          m_trackerdataFragment = new TrackerDataFragment(m_fragment->payload<const uint32_t*>(), m_fragment->payload_size());
          DEBUG("unpacking Tracker data fragment.");
          break;
        default:
          m_rawFragment=m_fragment->payload<const RawFragment*>();
          DEBUG("unpacking raw fragment.");
      }
      break;
    }
    case CalibrationTag:
      m_rawFragment=m_fragment->payload<const RawFragment*>();
      DEBUG("unpacking calibration fragment.");
      break;
    case MonitoringTag:
      m_monitoringFragment=m_fragment->payload<const MonitoringFragment*>();
      DEBUG("unpacking monitoring fragment.");
      break;
    case TLBMonitoringTag:
      m_tlbmonitoringFragment= std::make_unique<TLBMonitoringFragment>(TLBMonitoringFragment(m_fragment->payload<const uint32_t*>(), m_fragment->payload_size()));
      DEBUG("unpacking TLB monitoring fragment.");
      break;
    default:
      ERROR("Specified tag not found.");
      dataStatus |= UnclassifiedError;
  }
  
  return dataStatus;
}

uint16_t MonitorBaseModule::unpack_full_fragment( DataFragment<daqling::utilities::Binary> &eventBuilderBinary) {
  auto dataStatus = unpack_full_fragment(eventBuilderBinary, m_sourceID);
  return dataStatus;
}

void MonitorBaseModule::setupHistogramManager() {

  INFO("Setting up HistogramManager.");
  //INFO("Socket: "<<m_connections.getStatSocket());

  m_histogrammanager = std::make_unique<HistogramManager>();

  auto statsURI = m_config.getMetricsSettings()["stats_uri"];
  if (statsURI != "" && statsURI != nullptr) {
    INFO("Stats uri provided. Will publish histograms via the stats connection.");
    try {
      m_histogrammanager->configure(1, statsURI); // NOTE 1 I/O thread, and using same statsURI as for metrics. should be fine..
    } catch (std::exception &e){
      m_status = STATUS_ERROR;
      sleep(1); // wait for error state to appear in RC GUI.
      throw ConfigurationIssue(ERS_HERE, "Configuring histogram manager failed.");
    }
    m_histogramming_on = true;
  }
  else {
    INFO("No stats uri given. Will not be publishing histograms.");
  }

  return ;

}

void MonitorBaseModule::fill_error_status_to_metric( uint32_t fragmentStatus ) {

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
  
  return ;

}

void MonitorBaseModule::fill_error_status_to_histogram( uint32_t fragmentStatus, std::string hist_name ) {

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
  
  return ;

}

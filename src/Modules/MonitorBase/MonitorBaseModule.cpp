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

   auto cfg = getModuleSettings();
   auto cfg_sourceID = cfg["fragmentID"];
   if (cfg_sourceID!="" && cfg_sourceID!=nullptr)
      m_sourceID = cfg_sourceID;
   else m_sourceID=0;
   m_active_mon_lhc_modes={};
   m_bobrProcessThread =nullptr;
   m_activate_monitoring=false;

 }

MonitorBaseModule::~MonitorBaseModule() { 

  delete m_event;
  if (m_trackerdataFragment) delete m_trackerdataFragment;

  INFO("With config: " << m_config.dump());
}

void MonitorBaseModule::configure() {
  INFO("Configuring...");

  FaserProcess::configure();

  m_filter_physics = false;
  m_filter_random = false;
  m_filter_led = false;
  auto mod_cfgs = getModuleSettings();
  if (mod_cfgs.contains("TriggerFilter")){
   auto filter = mod_cfgs["TriggerFilter"].get<std::string>();
   if (filter == "Physics"){
     m_filter_physics = true;
     INFO("Monitoring physics triggered events only.");
   }
   else if (filter == "Random") {
     m_filter_random = true;
     INFO("Monitoring random triggered events only.");
   }
   else if (filter == "LED") {
     m_filter_led = true;
     INFO("Monitoring LED triggered events only.");
   }
   else{
     WARNING("TriggerFilter setting not recognised.");
   }
  } else INFO("No special trigger filter set. Monitoring all received events.");

  if (mod_cfgs.contains("publish_interval")){
    m_PUBINT = mod_cfgs["publish_interval"].get<int>();
  } else m_PUBINT = 5;

  if (mod_cfgs.contains("ActiveLHCModes")){
    m_active_mon_lhc_modes = mod_cfgs["ActiveLHCModes"].get<std::vector<int>>();
  }

  register_metrics();

  m_histogramming_on = false;
  setupHistogramManager();
  register_hists();

  // check if waiting for BOBR data - lets reserve channel 1 for BOBR data
  if (m_config.getNumReceiverConnections(getName())>1) {
    INFO("Will store BOBR data during run...");
    m_store_bobr_data=true;
  }
  else {
    INFO("Won't be storing BOBR data...");
    m_store_bobr_data = false;
  }

  return;
}

void MonitorBaseModule::start(unsigned int run_num) {
  FaserProcess::start(run_num);

  m_status = STATUS_OK;
  if ( m_histogramming_on ) m_histogrammanager->start();
  m_lhc_machinemode=-1;
  if (m_store_bobr_data) {
    if (m_bobrProcessThread != nullptr) {
      WARNING("BOBR data processing thread already started or still running. Call StopReadout() to stop old thread.");
    }
    else m_bobrProcessThread = new std::thread(&MonitorBaseModule::process_bobr_data, this);
  }


}

void MonitorBaseModule::stop() {
  FaserProcess::stop();

  INFO("... finalizing ...");

  if (m_bobrProcessThread != nullptr){
    if (!m_bobrProcessThread->joinable()){
      ERROR("BOBR data processing thread is not joinable!");
    }
    m_bobrProcessThread->join();
    delete m_bobrProcessThread;
    m_bobrProcessThread = nullptr;
  }

  if (m_histogramming_on) m_histogrammanager->stop();

}

void MonitorBaseModule::runner() noexcept {
  INFO("Running...");

  DataFragment<daqling::utilities::Binary> eventBuilderBinary;

  while (m_run) {

      m_event_header_unpacked = false;

      if ( !m_connections.receive(0, eventBuilderBinary)){
          std::this_thread::sleep_for(10ms);
          continue;
      }
      if (m_store_bobr_data && !m_activate_monitoring) continue;
      //DEBUG("Received event with size "<<eventBuilderBinary.size());
      try {
        if (m_filter_physics && !is_physics_triggered(eventBuilderBinary)) continue;
        if (m_filter_random && !is_random_triggered(eventBuilderBinary)) continue;
        if (m_filter_led && !is_led_triggered(eventBuilderBinary)) continue;
        monitor(eventBuilderBinary);
      } catch (UnpackDataIssue &e) {
        ERROR("Error checking data packet: "<<e.what()<<" Skipping event!");
      }

  }


  INFO("Runner stopped");

  return;

}

void MonitorBaseModule::process_bobr_data() noexcept {

  INFO("Running...");

  float check_point(0);
  DataFragment<daqling::utilities::Binary> bobrEventBinary;

  while (m_run) {
      if (!m_connections.receive(1, bobrEventBinary)){
        std::this_thread::sleep_for(1000ms);
        continue;
      }
      if (std::difftime(std::time(nullptr), check_point) > m_INTERVAL_BOBRUPDATE) {
        DEBUG("Received BOBR data");
        auto status = store(bobrEventBinary);
        if (status) m_status = STATUS_WARN;
        check_point = std::time(nullptr);
      }
  }

}

void MonitorBaseModule::monitor(DataFragment<daqling::utilities::Binary>&) {
}

void MonitorBaseModule::register_metrics() {

 INFO("... registering metrics in base MonitorBaseModule class ... " );

}

bool MonitorBaseModule::is_physics_triggered(DataFragment<daqling::utilities::Binary>& eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) throw UnpackDataIssue(ERS_HERE, "Error unpacking event header.");

  auto trig_bits = m_event->trigger_bits();
  if ( 0xF & trig_bits) return true;
  else return false; 

}

bool MonitorBaseModule::is_random_triggered(DataFragment<daqling::utilities::Binary>& eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) throw UnpackDataIssue(ERS_HERE, "Error unpacking event header.");

  auto trig_bits = m_event->trigger_bits();
  if ( 0x10 & trig_bits) return true;
  else return false; 

}

bool MonitorBaseModule::is_led_triggered(DataFragment<daqling::utilities::Binary>& eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) throw UnpackDataIssue(ERS_HERE, "Error unpacking event header.");

  auto trig_bits = m_event->trigger_bits();
  if ( 0x20 & trig_bits) return true;
  else return false; 

}

void MonitorBaseModule::register_error_metrics() {

   m_metric_payload=0;
  if ( m_stats_on ) {
    INFO("... registering error metrics ... " );
    registerVariable(m_metric_error_unclassified, "error_unclassified", metrics::ACCUMULATE);
    registerVariable(m_metric_error_bcidmismatch, "error_bcidmismatch", metrics::ACCUMULATE);
    registerVariable(m_metric_error_tagmismatch, "error_tagmismatch", metrics::ACCUMULATE);
    registerVariable(m_metric_error_timeout, "error_timeout", metrics::ACCUMULATE);
    registerVariable(m_metric_error_overflow, "error_overflow", metrics::ACCUMULATE);
    registerVariable(m_metric_error_corrupted, "error_corrupted", metrics::ACCUMULATE);
    registerVariable(m_metric_error_dummy, "error_dummy", metrics::ACCUMULATE);
    registerVariable(m_metric_error_unpack, "error_unpack", metrics::ACCUMULATE);
    registerVariable(m_metric_error_missing, "error_missing", metrics::ACCUMULATE);
    registerVariable(m_metric_error_empty, "error_empty", metrics::ACCUMULATE);
    registerVariable(m_metric_error_duplicate, "error_duplicate", metrics::ACCUMULATE);
  }
  return;

}

void MonitorBaseModule::register_hists() {

 INFO(" ... No histograms to register. Will not start histogram service. " );
 m_histogramming_on = false;

}

uint16_t MonitorBaseModule::store( DataFragment<daqling::utilities::Binary> &bobrEventBinary ) {
  uint16_t dataStatus(0);

  try {
    EventFull event(bobrEventBinary.data<const uint8_t*>(),bobrEventBinary.size());
    const EventFragment* fragment=event.find_fragment(DAQFormats::SourceIDs::BOBRSourceID);
    if (fragment==0) {
      ERROR("No correct fragment source ID found.");
      return dataStatus |= MissingFragment;
    }
    BOBRDataFragment bobr_data_frag = BOBRDataFragment(fragment->payload<const uint32_t*>(), fragment->payload_size());
    if (m_lhc_machinemode<0 || m_lhc_machinemode!=bobr_data_frag.machinemode()){
      m_lhc_machinemode= bobr_data_frag.machinemode();
      if (std::find(m_active_mon_lhc_modes.begin(), m_active_mon_lhc_modes.end(),m_lhc_machinemode)!=m_active_mon_lhc_modes.end()){
        INFO("Activating monitoring for machine mode "<<m_lhc_machinemode);
        m_activate_monitoring=true;
      }
      else { 
        INFO("Deactivating monitoring for machine mode "<<m_lhc_machinemode);
        m_activate_monitoring=false;
      }
    }
  } catch (const std::runtime_error& e) {
    ERROR(e.what());
    return dataStatus |= CorruptedFragment;
  }
  return  dataStatus;

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

TrackerDataFragment MonitorBaseModule::get_tracker_data_fragment(DataFragment<daqling::utilities::Binary> &eventBuilderBinary, uint8_t boardId) {

  if (unpack_fragment_header(eventBuilderBinary, TrackerSourceID+boardId)) throw UnpackDataIssue(ERS_HERE, "Issue encountered while unpacking fragment information.");

  return TrackerDataFragment(m_fragment->payload<const uint32_t*>(), m_fragment->payload_size());
}

TLBDataFragment MonitorBaseModule::get_tlb_data_fragment(DataFragment<daqling::utilities::Binary> &eventBuilderBinary) {

  if (m_eventTag == TLBMonitoringTag) throw UnpackDataIssue(ERS_HERE,"Can't retrieve TLB data fragment: This is a monitoring event!");
  if (unpack_fragment_header(eventBuilderBinary, TriggerSourceID)) throw UnpackDataIssue(ERS_HERE, "Issue encountered while unpacking fragment information.");

  return TLBDataFragment(m_fragment->payload<const uint32_t*>(), m_fragment->payload_size());
}

DigitizerDataFragment MonitorBaseModule::get_digitizer_data_fragment(DataFragment<daqling::utilities::Binary> &eventBuilderBinary) {

  if (unpack_fragment_header(eventBuilderBinary, PMTSourceID)) throw UnpackDataIssue(ERS_HERE, "Issue encountered while unpacking fragment information.");

  return DigitizerDataFragment(m_fragment->payload<const uint32_t*>(), m_fragment->payload_size());
}

TLBMonitoringFragment MonitorBaseModule::get_tlb_monitoring_fragment(DataFragment<daqling::utilities::Binary> &eventBuilderBinary) {

  if (m_eventTag != TLBMonitoringTag) throw UnpackDataIssue(ERS_HERE,"Can't retrieve TLB monitoring fragment: This is not a TLB monitoring event!");
  if (unpack_fragment_header(eventBuilderBinary, TriggerSourceID)) throw UnpackDataIssue(ERS_HERE, "Issue encountered while unpacking fragment information.");

  return TLBMonitoringFragment(m_fragment->payload<const uint32_t*>(), m_fragment->payload_size());
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

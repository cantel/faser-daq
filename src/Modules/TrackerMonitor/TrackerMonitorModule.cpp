/// \cond
#include <chrono>
#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream
/// \endcond

#include "TrackerMonitorModule.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;

TrackerMonitorModule::TrackerMonitorModule(): m_prefix_hname_hitp("hitpattern_mod") { 

   INFO("");
 }

TrackerMonitorModule::~TrackerMonitorModule() { 
  INFO("With config: " << m_config.dump());
 }

void TrackerMonitorModule::monitor(daqling::utilities::Binary &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  if ( m_event->event_tag() != m_eventTag ) {
    ERROR("Event tag does not match filter tag. Are the module's filter settings correct?");
    return;
  }

 auto fragmentUnpackStatus = unpack_full_fragment( eventBuilderBinary, SourceIDs::TriggerSourceID );
 if ( fragmentUnpackStatus ) {
    fill_error_status_to_metric( fragmentUnpackStatus );
    return;
  }
 if (m_tlbdataFragment->tbp() & 0x10) return; // ignore random triggered events

  //auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary); // if only monitoring information in header.
  fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary);
  if ( fragmentUnpackStatus ) {
    fill_error_status_to_metric( fragmentUnpackStatus );
    return;
  }

  uint32_t fragmentStatus = m_fragment->status();
  fill_error_status_to_metric( fragmentStatus );

  if ( m_trackerdataFragment ){
    if (m_trackerdataFragment->has_trb_error()) m_histogrammanager->fill("track_data_error_types", "TRBError");
    if (m_trackerdataFragment->has_module_error()) m_histogrammanager->fill("track_data_error_types", "ModuleError");
    if (m_trackerdataFragment->missing_event_id()) m_histogrammanager->fill("track_data_error_types", "NoEventID");
    if (m_trackerdataFragment->missing_bcid()) m_histogrammanager->fill("track_data_error_types", "NoBCID");
    if (m_trackerdataFragment->missing_crc()) m_histogrammanager->fill("track_data_error_types", "NoCRC");
    if (m_trackerdataFragment->missing_frames()) m_histogrammanager->fill("track_data_error_types", "MissingFrames");
    if (m_trackerdataFragment->unrecognized_frames()) m_histogrammanager->fill("track_data_error_types", "UnrecognizedFrames");
  }
  else {WARNING("Ignoring empty data fragment.");return;}

  if (m_trackerdataFragment->valid()){
    m_bcid = m_trackerdataFragment->bc_id();
    m_histogrammanager->fill("bcid", m_bcid); 
  }
  else {WARNING("Ignoring corrupted data fragment.");return;}


  for ( auto it = m_trackerdataFragment->cbegin(); it != m_trackerdataFragment->cend(); ++it ){
      auto sctEvent = *it;
      if (sctEvent == nullptr) { WARNING("Invalid SCT Event. Skipping."); continue;}
      std::string hname_hitp = m_prefix_hname_hitp+std::to_string(sctEvent->GetModuleID());
      m_histogrammanager->fill("diff_trb_sct_bcid", ((m_bcid%256)-sctEvent->GetBCID()));
      auto allHits = sctEvent->GetHits();
      for ( unsigned chipIdx = 0; chipIdx < (unsigned)kCHIPS_PER_MODULE*0.5; chipIdx++) {
        auto hitsPerChip1 = allHits[chipIdx];
        for (auto hit1 : hitsPerChip1){ // hit is an std::pair<uint8 strip, uint8 pattern>
          if ( hit1.second == 7 ) continue;
          auto strip1 = hit1.first;
          auto hitsPerChip2 = allHits[kCHIPS_PER_MODULE - 1 - chipIdx];
          for (auto hit2 : hitsPerChip2){ // hit is an std::pair<uint8 strip, uint8 pattern>
            if ( hit2.second == 7 ) continue;
            auto strip2 = kSTRIPS_PER_CHIP-1-hit2.first; // invert
            if ( std::abs(strip1-strip2) > kSTRIPDIFFTOLERANCE ) continue;
            // good physics hits
            std::bitset<3> bitset_hitp1(hit1.second);
            m_histogrammanager->fill(hname_hitp, bitset_hitp1.to_string());
            std::bitset<3> bitset_hitp2(hit2.second);
            m_histogrammanager->fill(hname_hitp, bitset_hitp2.to_string());
          }
        }
      }
  }


}

void TrackerMonitorModule::register_hists() {

  INFO(" ... registering histograms in TrackerMonitor ... " );

  m_histogrammanager->registerHistogram("bcid", "BCID", 0, 3564, 3564, 30);
  m_histogrammanager->registerHistogram("diff_trb_sct_bcid", "TRB BCID - SCT BCID", 10, -5, 5, Axis::Range::EXTENDABLE, 10);

  // per module
  std::vector<std::string> hitp_categories = { "000", "001", "010", "011", "100", "110", "111" };
  for ( unsigned i = 0; i < 8; i++ ){
    std::string hname_hitp = m_prefix_hname_hitp+std::to_string(i);
    m_histogrammanager->registerHistogram(hname_hitp, "hit pattern", hitp_categories, 10);
  }

  std::vector<std::string> error_categories = {"TRBError", "ModuleError", "NoEventID", "NoBCID", "NoCRC", "MissingFrames", "UnrecognizedFrames" };
  m_histogrammanager->registerHistogram("track_data_error_types", "error type", error_categories, 10);

  INFO(" ... done registering histograms ... " );

  return ;

}

void TrackerMonitorModule::register_metrics() {

  INFO( "... registering metrics in TrackerMonitorModule ... " );

  register_error_metrics();

  m_metric_payload = 0;
  m_statistics->registerMetric(&m_metric_payload, "payload", daqling::core::metrics::LAST_VALUE);

  return;
}

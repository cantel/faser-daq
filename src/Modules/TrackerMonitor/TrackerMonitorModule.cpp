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

TrackerMonitorModule::TrackerMonitorModule() { 

   INFO("");
   m_decoder = new FASER::TRBEventDecoder();
 }

TrackerMonitorModule::~TrackerMonitorModule() { 
  INFO("With config: " << m_config.dump() << " getState: " << this->getState());
 }

void TrackerMonitorModule::monitor(daqling::utilities::Binary &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  if ( m_event->event_tag() != m_eventTag ) {
    ERROR("Event tag does not match filter tag. Are the module's filter settings correct?");
    return;
  }

  auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary); // if only monitoring information in header.
  if ( fragmentUnpackStatus ) {
    fill_error_status_to_metric( fragmentUnpackStatus );
    return;
  }
  // m_rawFragment or m_monitoringFragment should now be filled, depending on tag.

  uint32_t fragmentStatus = m_fragment->status();
  fill_error_status_to_metric( fragmentStatus );

  m_histogrammanager->fill("bcid", m_fragment->bc_id()); 

  // ----- Get module data using TRB event decoder ---- 
  //

  std::vector<uint32_t> rawpayload = m_fragment->rawPayload();
  if (!(m_decoder->IsEndOfDAQ(rawpayload.at(0)))){
  m_decoder->LoadTRBEventData(rawpayload);
  std::vector<FASER::TRBEvent*> decoded_event = m_decoder->GetEvents(); 
  if (decoded_event.size() != 0){
      auto local_event_id = decoded_event[0]->GetL1ID();
      auto trb_bc_id = decoded_event[0]->GetBCID();
      unsigned int nmodules = decoded_event[0]->GetNModulesPresent();
      for ( unsigned int i = 0; i < nmodules; i++ ){
        FASER::SCTEvent* sctevent= decoded_event[0]->GetModule(i);
        if (sctevent != nullptr){
          m_histogrammanager->fill2D("trbbcid_vs_modulebcid", trb_bc_id, sctevent->GetBCID());
         } // sct event valid
       } // module loop
    } // event exists.
  } // data is not EndOfDAQ (shouldn't be ncessary)

}

void TrackerMonitorModule::register_hists() {

  INFO(" ... registering histograms in TrackerMonitor ... " );

  m_histogrammanager->registerHistogram("bcid", "BCID", -0.5, 3564.5, 3565, 3600);

  m_histogrammanager->register2DHistogram("trbbcid_vs_modulebcid", "TRB BCID", -0.5, 3564.5, 3565, "SCT module BCID", -0.5, 255.5, 256, 3600);

  INFO(" ... done registering histograms ... " );

  return ;

}

void TrackerMonitorModule::register_metrics() {

  INFO( "... registering metrics in TrackerMonitorModule ... " );

  register_error_metrics();

  return;
}

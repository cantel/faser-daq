/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

/// \cond
#include <chrono>
#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream
#include "Utils/Ers.hpp"
/// \endcond


#include "TCalibMonitorModule.hpp"
//using //namespace chrono::chrono_literals;
//using //namespace chrono::chrono;



TCalibMonitorModule::TCalibMonitorModule(const std::string& n): MonitorBaseModule(n),m_prefix_hname_hitp("hitpattern_physics_mod"),m_hname_scterrors("sct_data_errors"){
   INFO("TCalibMonitorModule()");
   auto cfg = getModuleSettings();
   auto cfg_trigbits_select = cfg["PhysicsTriggerBits"];
   if (cfg_trigbits_select!="" && cfg_trigbits_select!=nullptr)
      m_physics_trigbits = cfg_trigbits_select;
   else m_physics_trigbits=0xf;

}

TCalibMonitorModule::~TCalibMonitorModule() { 
  INFO("With config: " << m_config.dump());
}

void TCalibMonitorModule::start(unsigned int run_num){
  INFO("Starting "<<getName());
  MonitorBaseModule::start(run_num);
}



void TCalibMonitorModule::monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  if ( m_event->event_tag() != m_eventTag ) {
    ERROR("Event tag does not match filter tag. Are the module's filter settings correct?");
    return;
  }

  bool randTrig(false);
  bool physicsTrig(false);
  auto fragmentUnpackStatus = unpack_full_fragment( eventBuilderBinary, SourceIDs::TriggerSourceID );
  if ( fragmentUnpackStatus ) {
     WARNING("Unpacking error for trigger fragment. Can't use trigger information for this event!");
  } else {
    if (m_tlbdataFragment->tbp() & 0x10) randTrig = true;
    if ((m_tlbdataFragment->tbp()&0xf) & m_physics_trigbits) physicsTrig=true;
  }


  if ( m_trackerdataFragment ){
    if (m_trackerdataFragment->has_trb_error()) m_histogrammanager->fill("track_data_error_types", "TRBError");
    if (m_trackerdataFragment->has_module_error()) m_histogrammanager->fill("track_data_error_types", "ModuleError");
    if (m_trackerdataFragment->missing_event_id()) m_histogrammanager->fill("track_data_error_types", "NoEventID");
    if (m_trackerdataFragment->missing_bcid()) m_histogrammanager->fill("track_data_error_types", "NoBCID");
    if (m_trackerdataFragment->missing_crc()) m_histogrammanager->fill("track_data_error_types", "NoCRC");
    if (m_trackerdataFragment->missing_frames()) m_histogrammanager->fill("track_data_error_types", "MissingFrames");
    if (m_trackerdataFragment->unrecognized_frames()) m_histogrammanager->fill("track_data_error_types", "UnrecognizedFrames");
    if (!m_trackerdataFragment->valid()) {
      WARNING("Errors were found in tracker fragment:\n"<<*m_trackerdataFragment<<"\n Skipping event!");
      m_metric_total_errors++;
      return;
    }
  }
  else {WARNING("Ignoring empty data fragment.");return;}

  size_t payload_size = m_fragment->payload_size();
  if ( payload_size > MAXFRAGSIZE ) {
     WARNING(" VERY large payload size received. Payload size of "<<payload_size<<" bytes exceeds maximum allowable for histogram filling. Resetting to "<<MAXFRAGSIZE);
     payload_size = MAXFRAGSIZE;
  } 
  m_histogrammanager->fill("payloadsize", payload_size);

  m_bcid = m_trackerdataFragment->bc_id();
  m_l1id = m_trackerdataFragment->event_id();
  m_histogrammanager->fill("bcid", m_bcid);

  std::vector<int> hitsPerModule;
  hitsPerModule.resize(8);

  for ( auto it = m_trackerdataFragment->cbegin(); it != m_trackerdataFragment->cend(); ++it ){
      auto sctEvent = *it;
      if (sctEvent == nullptr) { 
        WARNING("Invalid SCT Event for event "<<m_trackerdataFragment->event_id());
        WARNING("tracker data fragment: "<<*m_trackerdataFragment);
        m_status = STATUS_WARN;
        m_histogrammanager->fill("track_data_error_types", "ModuleDecodeError");
        continue;
      }

      // how about SCT data errors
     std::string hname_mod_scterrors = m_hname_scterrors+"_mod"+std::to_string(sctEvent->GetModuleID());
     if ( sctEvent->HasError() ) {
        auto sctErrorList= sctEvent->GetErrors();
        for ( unsigned idx = 0; idx < 12; idx++ ) {
          auto sctErrors = sctErrorList.at(idx);
          for ( uint8_t sctError : sctErrors ) {
            WARNING("SCT Error for module "<<sctEvent->GetModuleID()<<", chip idx "<<idx<<". Error code = "<<static_cast<int>(sctError));
            //m_total_WARNINGS++;
            switch ( sctError ) {
              case 0x1:
               m_histogrammanager->fill(hname_mod_scterrors, "NoData");
               m_histogrammanager->fill(m_hname_scterrors, "NoData");
               break;
              case 0x2:
               m_histogrammanager->fill(hname_mod_scterrors, "BuffOverflow");
               m_histogrammanager->fill(m_hname_scterrors, "BuffOverflow");
               break;
              case 0x4:
               m_histogrammanager->fill(hname_mod_scterrors, "BuffError");
               m_histogrammanager->fill(m_hname_scterrors, "BuffError");
               break;
              case 0xff:
               m_histogrammanager->fill(hname_mod_scterrors, "UnknownChip");
               m_histogrammanager->fill(m_hname_scterrors, "UnknownChip");
               break;
              default:
               m_histogrammanager->fill(hname_mod_scterrors, "Unknown");
               m_histogrammanager->fill(m_hname_scterrors, "Unknown");
            }
          }
        }
      }; // end of SCT ERRORs check



      uint8_t module = sctEvent->GetModuleID();
      auto allHits = sctEvent->GetHits();

      if (physicsTrig && m_lhc_physics_mode) { // only selected physics triggered events during LHC collisions
        //std::string hname_hitp = m_prefix_hname_hitp+std::to_string(sctEvent->GetModuleID());
        for ( unsigned chipIdx = 0; chipIdx < (unsigned)kCHIPS_PER_MODULE*0.5; chipIdx++) {
          auto hitsPerChip1 = allHits[chipIdx];
          for (auto hit1 : hitsPerChip1){ // hit is an std::pair<uint8 strip, uint8 pattern>
            if ( hit1.second == 7 ) continue;
            unsigned chipIdx2 = kCHIPS_PER_MODULE - 1 - chipIdx;
            m_histogrammanager->fill2D("hitmap_physics", module, chipIdx, 1);
            m_histogrammanager->fill2D("hitmap_physics", module, chipIdx2, 1);
          }
        }

      }


  //uint8_t module = sctEvent->GetModuleID();
    if (randTrig) {
          for ( unsigned chipIdx = 0; chipIdx < 12; chipIdx++) {
            auto hitsPerChip = allHits[chipIdx];
            for (auto hit : hitsPerChip){
                std::bitset<3> bitset_hitp(hit.second);
                m_histogrammanager->fill("hitpattern_random", bitset_hitp.to_string());
                if ( hit.second == 7 || hit.second == 0 ) continue;
                  m_histogrammanager->fill2D("hitmap_random", module, chipIdx, 1);
            
            }
          }
        }

    	
    if (sctEvent != nullptr){
      auto threshold = getModuleSettings()["threshold"]; //is this right?
      
      hitsPerModule[module] += sctEvent->GetNHits();	      
      auto hits = sctEvent->GetHits(); 
      int chipcnt(0);
      for(auto hit : hits){ 
        for(auto h : hit){ 
          int strip = (int)h.first;		  
          int link = (int)(chipcnt/6);		  
          int ichip = link > 0 ? chipcnt - 6 : chipcnt;
          int ith=0; //placeholder, how do i get threshold from sctevent? or is this 2 different m_hits?
          std::string hname_mod_thr_hits = "maskscan_mod"+std::to_string(module)+"_thr"+std::to_string(0);
          m_histogrammanager->fill2D(hname_mod_thr_hits, strip, link, 1);
          //m_hits[ith][imodule][link][ichip][strip]++;		
              }
        chipcnt++;
      }	    
    }
  }
}

void TCalibMonitorModule::register_hists() {

  INFO(" ... registering histograms in SCTDataMonitor ... " );

  m_histogrammanager->registerHistogram("payloadsize", "payload size [bytes]", 0, MAXFRAGSIZE/50, MAXFRAGSIZE/2000,  Axis::Range::EXTENDABLE, m_PUBINT*10);

  m_histogrammanager->registerHistogram("bcid", "BCID", -0.5, 3564.5, 3565, 1800);
  m_histogrammanager->register2DHistogram("hitmap_physics", "module idx", 0, kTOTAL_MODULES, kTOTAL_MODULES, "chip idx",  0, kCHIPS_PER_MODULE, kCHIPS_PER_MODULE, m_PUBINT);
  m_histogrammanager->register2DHistogram("hitmap_random", "module idx", 0, kTOTAL_MODULES, kTOTAL_MODULES, "chip idx",  0, kCHIPS_PER_MODULE, kCHIPS_PER_MODULE, m_PUBINT);

  //Attempt m_hits storage
  m_histogrammanager->register2DHistogram("hitmap_physics_th0", "module idx", 0, kTOTAL_MODULES, kTOTAL_MODULES, "chip idx",  0, kCHIPS_PER_MODULE, kCHIPS_PER_MODULE, m_PUBINT);

  std::vector<std::string> trb_error_categories = {"TRBError", "ModuleError", "NoEventID", "NoBCID", "NoCRC", "MissingFrames", "UnrecognizedFrames", "ModuleDecodeError"};
  m_histogrammanager->registerHistogram("track_data_error_types", "error type", trb_error_categories, m_PUBINT);

  std::vector<std::string> sct_error_categories = {"L1IDMismatch", "BCIDMismatch", "NoData", "BuffOverflow", "BuffError", "UnknownChip", "Unknown"};
  for ( unsigned i = 0; i < kTOTAL_MODULES; i++ ){
    std::string hname_mod_scterr = m_hname_scterrors+"_mod"+std::to_string(i);
    m_histogrammanager->registerHistogram(hname_mod_scterr, "error type", sct_error_categories, m_PUBINT);
  }
  m_histogrammanager->registerHistogram(m_hname_scterrors, "error type", sct_error_categories, m_PUBINT); 

  for(int mod = 0; mod < kTOTAL_MODULES; mod ++){
    for(int thr =0; thr<MASKMAXTHR; thr++){
      //how to format a string???? use concatenation see line 218
        std::string hname_mod_thr_hits = "maskscan_mod"+std::to_string(mod)+"_thr"+std::to_string(thr);
        int MAXCHANNELS = kSTRIPS_PER_CHIP*NCHIPS;
         m_histogrammanager->register2DHistogram(hname_mod_thr_hits, "Channel #", 0, MAXCHANNELS, MAXCHANNELS, "Link",  0, NLINKS, NLINKS, m_PUBINT);  }
    }
          INFO(" ... done registering histograms ... " );

  return ;

}


/*

void TCalibMonitorModule::foobar(const std::string &arg) {
  ERS_INFO("Inside custom command. Got argument: " << arg);
}

*/
/*
( std::string name, "hitmap_random",

std::string xlabel : "module idx"

float xmin, float xmax : 0, kTOTAL_MODULES

unsigned int xbins, kTOTAL_MODULES,

std::string ylabel, "chip idx",

float ymin, float ymax, :  0, kCHIPS_PER_MODULE,

 unsigned int ybins,  kCHIPS_PER_MODULE

unsigned int delta_t = kMIN_INTERVAL , m_PUBINT);

register a histogram for every module.
inside those histograms is strips/channels and noisy/dead status


*/

/*
FROM OLD CODE
m_hits requires threshold, module, link, chip, strip

FASER::TRBEventDecoder *ed = new FASER::TRBEventDecoder();
      ed->LoadTRBEventData(trb->GetTRBEventData());      
      auto evnts = ed->GetEvents();
      std::vector<int> hitsPerModule;
      hitsPerModule.resize(8);	
      for (auto evnt : evnts){ 	
        for (unsigned int imodule=0; imodule<8; imodule++){
          auto sctEvent = evnt->GetModule(imodule);	  
          if (sctEvent != nullptr){
            hitsPerModule[imodule] += sctEvent->GetNHits();	      
            auto hits = sctEvent->GetHits(); 
            int chipcnt(0);
            for(auto hit : hits){ 
              for(auto h : hit){ 
                int strip = (int)h.first;		  
                int link = (int)(chipcnt/6);		  
                int ichip = link > 0 ? chipcnt - 6 : chipcnt;
                m_hits[ith][imodule][link][ichip][strip]++;		
                    }
              chipcnt++;
            }	    
          }
        }// end loop in modules 
      }// end loop in events
*/

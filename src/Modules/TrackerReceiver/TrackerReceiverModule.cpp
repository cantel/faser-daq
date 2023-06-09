/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#include "TrackerReceiverModule.hpp"
#include "TrackerReadout/TRBAccess.h"
#include "Commons/FaserProcess.hpp"
#include "EventFormats/DAQFormats.hpp"
#include <Utils/Binary.hpp>
#include "TrackerReadout/ConfigurationHandling.h"
#include "EventFormats/TrackerDataFragment.hpp"
#include <string>
#include <iostream>
#include <ctime>

using namespace DAQFormats;
using namespace daqling::utilities;
using namespace TrackerReceiverIssues;
using namespace TrackerData;
TrackerReceiverModule::TrackerReceiverModule(const std::string& n):FaserProcess(n) { 
    INFO("");
    m_status = STATUS_OK;

    auto cfg = getModuleSettings();

    bool runEmulation = getModuleSettings()["emulation"]; 

    if ( cfg.contains("BoardID")) {
      m_userBoardID = cfg["BoardID"];
    }
    else if (!runEmulation) {
      throw NoBoardID(ERS_HERE);
    }

    bool usingUSB(false);
    if ( cfg.contains("SCIP")) {
      m_SCIP = cfg["SCIP"];
      if ( cfg.contains("DAQIP")) {
        m_DAQIP = cfg["DAQIP"];
      }
      else {
        throw NoDaqIP(ERS_HERE);
      }
      INFO("SC and DAQ IPs have been specified. Assuming we're communicating through the ether!");
    }
    else {
       INFO("**NO** SC and DAQ IPs have been specified. Assuming we're communicating via USB!");
       usingUSB = true;
    }

    m_extClkSelect = true;
    if ( cfg.contains("L1Atype")) {
      if (cfg["L1Atype"] == "internal" ) m_extClkSelect = false;
    }

    if ( cfg.contains("ReadoutMode")) {
      m_ABCD_ReadoutMode = cfg["ReadoutMode"];
    }
    else {
      WARNING("No chip readout mode selected. Setting to HIT mode by default."); 
      m_ABCD_ReadoutMode = FASER::TRBAccess::ABCD_ReadoutMode::HIT;
    }

    if ( cfg.contains("EdgeDetect")) {
       m_ABCD_EdgeDetect = cfg["EdgeDetect"].get<bool>();
    }
    else {
      WARNING("No chip edge detect mode selected. Disabling edge detect by default."); 
      m_ABCD_EdgeDetect = false;
    }

    if ( cfg.contains("ConfigureModules")) {
      m_configureModules = cfg["ConfigureModules"];
    }
    else m_configureModules = true;

    if ( cfg.contains("HWDelayClk0")) {
       m_hwDelay_Clk0 = cfg["HWDelayClk0"];
    }
    else {WARNING("No Clk0 hardware delay setting provided. Setting to 0."); m_hwDelay_Clk0 = 0;}

    if ( cfg.contains("HWDelayClk1")) {
       m_hwDelay_Clk1 = cfg["HWDelayClk1"];
    }
    else {WARNING("No Clk1 hardware delay setting provided. Setting to 0."); m_hwDelay_Clk1 = 0;}

    if ( cfg.contains("FinePhaseClk0")) {
       m_finePhaseDelay_Clk0 = cfg["FinePhaseClk0"];
    }
    else {WARNING("No CLK0 fine phase delay setting provided. Setting to 0."); m_finePhaseDelay_Clk0 = 0;}

    if ( cfg.contains("FinePhaseClk1")) {
       m_finePhaseDelay_Clk1 = cfg["FinePhaseClk1"];
    }
    else {WARNING("No CLK1 fine phase delay setting provided. Setting to 0."); m_finePhaseDelay_Clk1 = 0;}

    if ( cfg.contains("FinePhaseLed0")) {
       m_finePhaseDelay_Led0 = cfg["FinePhaseLed0"];
    }
    else {WARNING("No LED0 fine phase delay setting provided. Setting to 0."); m_finePhaseDelay_Led0 = 0;}

    if ( cfg.contains("FinePhaseLed1")) {
       m_finePhaseDelay_Led1 = cfg["FinePhaseLed1"];
    }
    else {WARNING("No LED1 fine phase delay setting provided. Setting to 0."); m_finePhaseDelay_Led1 = 0;}

    if ( cfg.contains("RxTimeoutDisable")) {
       m_RxTimeoutDisable = cfg["RxTimeoutDisable"];
    }
    else m_RxTimeoutDisable = false;

    if ( cfg.contains("ApplySCTMasks")) {
       m_applySCTMasks = cfg["ApplySCTMasks"];
    }
    else m_applySCTMasks = false;

    INFO("\n\n*** Configurations for TrackerReceiver module *** \n emulation mode: "<<
          (runEmulation?"TRUE":"FALSE")<<
          "\n Running on CLK: "<<(m_extClkSelect?"TLB CLK":"INTERNAL CLK")<<
          "\n TRB board ID: "<<m_userBoardID<<
          "\n using USB: "<<(usingUSB?"TRUE (No SC/DAQ IP required)":"FALSE")<<
          "\n SC IP: "<<m_SCIP<<"\n DAQ IP: "<<m_DAQIP<<
          "\n chip readout mode: "<<m_ABCD_ReadoutMode<<
          "\n chip edge detect enabled: "<<(m_ABCD_EdgeDetect?"TRUE":"FALSE")<<
          "\n CLK0 fine phase delay: "<<m_finePhaseDelay_Clk0<<" \n CLK1 fine phase delay: "<<m_finePhaseDelay_Clk1<<
          "\n LED(X)0 fine phase delay: "<<m_finePhaseDelay_Led0<<"\n LED(X)1 fine phase delay: "<<m_finePhaseDelay_Led1<<
          "\n Configure modules: "<<(m_configureModules?"TRUE":"FALSE")<<
          "\n RxTimeoutDisable: "<<(m_RxTimeoutDisable?"TRUE":"FALSE")<<
          "\n debug mode: "<<(m_debug?"TRUE":"FALSE")<<"\n");

    if (usingUSB) m_trb = std::make_unique<FASER::TRBAccess>(0, runEmulation, m_userBoardID );
    else m_trb = std::make_unique<FASER::TRBAccess>(m_SCIP, m_DAQIP, 0, runEmulation, m_userBoardID );
    
    auto log_level = m_config.getConfig()["loglevel"]["module"];
    m_debug = (log_level=="DEBUG"?1:0);
    if (log_level == "TRACE") {  
      m_debug = true;
      m_trace = true;
      m_trb->SetDebug(true);
    }
    else {
      m_trb->SetDebug(false);    
    }

    //Setting up the emulated interface
    if (runEmulation) {
      std::string l_TRBconfigFile = getModuleSettings()["emulatorFile"];
      static_cast<FASER::dummyInterface*>(m_trb->m_interface)->SetInputFile(l_TRBconfigFile); //file to read events from
    }
}    

TrackerReceiverModule::~TrackerReceiverModule() { 
    INFO("Disable L1A, BCR and Trigger Clock, else modules can't be configured next time.");
    m_status = FASER::TRBAccess::GPIOCheck(m_trb.get(), &FASER::TRBAccess::SetDirectParam, 0); // disable L1A, BCR and Trigger Clock, else modules can't be configured next time.
    if (m_status){
      WARNING("Issue encountered while trying to unset direct parameters. This might cause problems during configurations next time. Continuing...");
    }
    INFO("Shutdown");
}


/***************************************
 *        Configure module
 * ************************************/
void TrackerReceiverModule::configure() {
  FaserProcess::configure();
  INFO("TRB --> configuration");

  registerVariable(m_physicsEventCount, "TriggeredEvents");
  registerVariable(m_physicsEventCount, "PhysicsRate", metrics::RATE);
  registerVariable(m_event_size_bytes, "event_size_bytes");
  registerVariable(m_corrupted_fragments, "BadFragments");
  registerVariable(m_corrupted_fragments, "BadFragmentsRate", metrics::RATE);
  registerVariable(m_checksum_mismatches, "checksum_mismatches");
  registerVariable(m_checksum_mismatches_rate, "checksum_mismatches_rate");
  registerVariable(m_receivedEvents, "ReceivedEvents"); // events transferred from driver to tracker receiver.
  registerVariable(m_dataRate, "DataRate", metrics::LAST_VALUE); // kB/s read via network socket
  registerVariable(m_PLLErrCnt, "PLLErrCnt", metrics::LAST_VALUE);
  registerVariable(m_missedL1, "MissedEventIDError");

  //TRB configuration 
  m_moduleMask = 0;
  m_moduleClkCmdMask = 0;
  for (int i = 7; i >= 0; i--){ //there are 8 modules
    if (getModuleSettings()["moduleMask"][i]){
      m_moduleMask |= (1 << i);
    }
    if (getModuleSettings()["moduleClkCmdMask"][i]){
      m_moduleClkCmdMask |= (1 << i);
    }
  }

  if (m_moduleMask > 0xff) {
    ERROR("We have at most 8 modules per TRB, but you specified 0x" <<std::hex<<m_moduleMask<< " as moduleMask!");
    return;
  }
  if (m_moduleClkCmdMask > 0xff) {
    ERROR("We have at most 8 modules per TRB, but you specified 0x" <<std::hex<<m_moduleMask<< " as moduleClkCmdMask!");
    return;
  }

  // disable all before configuring 
  m_trb->SetDirectParam(0); // disable L1A, BCR and Trigger Clock, else modules can't be configured next time.
  m_trb->GetConfig()->Set_Global_L2SoftL1AEn(0);
  m_trb->GetConfig()->Set_Module_L1En(0); 
  m_trb->GetConfig()->Set_Module_BCREn(0); 
  m_trb->GetConfig()->Set_Module_ClkCmdSelect(m_moduleClkCmdMask);
  m_trb->GetConfig()->Set_Module_LedRXEn(0);
  m_trb->GetConfig()->Set_Module_LedxRXEn(0);
  m_trb->WriteConfigReg();

  m_trb->GetPhaseConfig()->SetFinePhase_Clk0(m_finePhaseDelay_Clk0);
  m_trb->GetPhaseConfig()->SetFinePhase_Led0(m_finePhaseDelay_Led0);
  m_trb->GetPhaseConfig()->SetFinePhase_Clk1(m_finePhaseDelay_Clk1);
  m_trb->GetPhaseConfig()->SetFinePhase_Led1(m_finePhaseDelay_Led1);
  m_trb->WritePhaseConfigReg();
  m_trb->ApplyPhaseConfig();
 

  if ( m_configureModules ) {
    //Modules configuration
    for (int l_moduleNo=0; l_moduleNo < 8; l_moduleNo++){ //we have 8 modules per TRB
  
      if ((0x1 << l_moduleNo ) & m_moduleMask){ //checks if the module is active according to the module mask
      INFO("Starting configuration of module number " << l_moduleNo);
          
        std::unique_ptr<FASER::ConfigHandlerSCT> l_cfg(new FASER::ConfigHandlerSCT());
        if (getModuleSettings()["moduleConfigFiles"].contains(std::to_string(l_moduleNo))){ //module numbers in cfg file from 0 to 7!!
          std::string l_moduleConfigFile = getModuleSettings()["moduleConfigFiles"][std::to_string(l_moduleNo)];
          if (l_moduleConfigFile != ""){
              l_cfg->ReadFromFile(l_moduleConfigFile);
              l_cfg->SetReadoutMode(m_ABCD_ReadoutMode);
              l_cfg->SetEdgeDetection(m_ABCD_EdgeDetect);
              l_cfg->SetApplySCTMasks(m_applySCTMasks);
          }
          else { 
            m_status=STATUS_ERROR;
            sleep(1);
            throw MissingConfigurationFile(ERS_HERE,l_moduleNo);
          }
          try {
            m_trb->ConfigureSCTModule(l_cfg.get(), (0x1 << l_moduleNo)); //sending configuration to corresponding module
          } catch ( TRBConfigurationException &e) {
             m_status=STATUS_ERROR;
             sleep(1);
             throw TRBConfigurationIssue(ERS_HERE,e);
          }
          INFO("Configuration of module " << l_moduleNo << " finished.");
        }
        else{
          m_status=STATUS_ERROR;
          ERROR("Module " << l_moduleNo << " enabled by mask but no configuration file provided!");
        }
      }
    }
  }

  // first common configs
  m_trb->GetConfig()->Set_Global_RxTimeoutDisable(m_RxTimeoutDisable);
  m_trb->GetConfig()->Set_Global_L1TimeoutDisable(false);
  m_trb->GetConfig()->Set_Global_Overflow(4095);
  m_trb->GetConfig()->Set_Global_TLBClockSel(m_extClkSelect);
  m_trb->GetConfig()->Set_Global_L1AEn(m_extClkSelect);
  m_trb->GetConfig()->Set_Global_BCREn(m_extClkSelect);
  m_trb->GetConfig()->Set_Global_L2SoftL1AEn(!m_extClkSelect);
  m_trb->GetConfig()->Set_Global_HardwareDelay0(m_hwDelay_Clk0);
  m_trb->GetConfig()->Set_Global_HardwareDelay1(m_hwDelay_Clk1);
  m_trb->GetConfig()->Set_Module_LedRXEn(m_moduleMask);
  m_trb->GetConfig()->Set_Module_LedxRXEn(m_moduleMask);
  m_trb->GetConfig()->Set_Module_ClkCmdSelect(m_moduleClkCmdMask);
  if ( m_extClkSelect ) { // running on TLB clock
    m_trb->GetConfig()->Set_Module_L1En(m_moduleMask); 
    m_trb->GetConfig()->Set_Module_BCREn(m_moduleMask); 
    bool retry(true);
    int8_t nRetries(3);
    while (retry && nRetries--) {
      m_trb->SetDirectParam(FASER::TRBDirectParameter::TLBClockSelect);
      // give it a moment to sync with clock
      uint16_t status;
      for(unsigned int i = 5; i >0; i--){
        m_trb->ReadStatus(status);
        if (status & FASER::TRBStatusParameters::STATUS_TLBCLKSEL){ 
          INFO( "   TLB CLK synchronized");
          break;
        }
        INFO( "    Waiting to sync to TLB CLK ...");
        usleep(1e5); // 100 ms
      }
      if ( !(status & FASER::TRBStatusParameters::STATUS_TLBCLKSEL) ) {
        if (!nRetries) { m_status=STATUS_ERROR; sleep(1); throw TLBSyncFailed(ERS_HERE);}  
        continue;
      } 
      m_trb->WritePhaseConfigReg();
      m_trb->ApplyPhaseConfig();
      try {
        m_trb->GenerateSoftReset(m_moduleMask);
      } catch ( Exceptions::BaseException &e  ){ // FIXME figure out why it won't catch TRBAccessException
         if (!nRetries) { m_status=STATUS_ERROR; sleep(1); throw TRBAccesIssue(ERS_HERE,e); };
         ERROR("Sending configuration commands failed. Will try resyncing to clock. "<<(int)nRetries<<" retries remaining.");
         m_trb->SetDirectParam(0);
         uint16_t status;
         for(unsigned int i = 5; i >0; i--){
           m_trb->ReadStatus(status);
           if (status & FASER::TRBStatusParameters::STATUS_LOCALCLKSEL){
             INFO( "   Fell back to internal CLK");
             break;
            }
            INFO( "    Still falling back to internal CLK ...");
            usleep(1e5); // 100 ms
          }
          continue;
      }
      retry = false;
    }
  }
  else { //running on internal clock
    m_trb->GenerateSoftReset(m_moduleMask);
  }

  m_trb->WriteConfigReg();

  INFO("TRB configurations set by user:");
  m_trb->GetConfig()->Print();
  try { m_trb->VerifyConfigReg(); }
  catch ( TRBConfigurationException& e) {
    m_status=STATUS_ERROR;
    ERROR("Configurations read back do not match configurations sent. Reason: "<<e);
  }
  INFO("TRB configured successfully. TRB configurations read back:");
  m_trb->ReadbackAndPrintConfig();


  m_trb->SCT_EnableDataTaking(m_moduleMask);
  m_trb->SetDirectParam(m_trb->GetConfig()->GetGlobalDirectParam());
  m_trb->PrintStatus();

}


void TrackerReceiverModule::sendECR()
{
  INFO("Received ECR command.");
  m_status = FASER::TRBAccess::GPIOCheck(m_trb.get(), &FASER::TRBAccess::L1CounterReset); // TODO need to reset SCT module counters here as well
  if (m_status) {
    ERROR("Issue encountered while resetting the TRB L1 counter. Try resend ECR?");
  }
  else INFO("ECR successful.");
  INFO("ECR count = " << m_ECRcount);
  m_prev_event_id = 0;
}


/***************************************
 *        Start module
 * ************************************/
void TrackerReceiverModule::start(unsigned run_num) {
  INFO("Starting...");
  uint16_t param = FASER::TRBReadoutParameters::READOUT_L1COUNTER_RESET | FASER::TRBReadoutParameters::READOUT_ERRCOUNTER_RESET | FASER::TRBReadoutParameters::READOUT_FIFO_RESET;
  m_status = FASER::TRBAccess::GPIOCheck(m_trb.get(), &FASER::TRBAccess::StartReadout, param); //doing ErrCnTReset, FifoReset,L1ACounterReset
  if (m_status){
    THROW(TRBAccessException, "Board communication issue encountered when starting readout.");
  }
  INFO("TRB --> readout started." );
  m_triggerEnabled = true;
  INFO("Starting software DAQ processing.");
  FaserProcess::start(run_num);
}


/***************************************
 *        Stop module
 * ************************************/
void TrackerReceiverModule::stop() {
  std::this_thread::sleep_for(std::chrono::microseconds(100000)); //wait for events 
  m_status = FASER::TRBAccess::GPIOCheck(m_trb.get(), &FASER::TRBAccess::StopReadout);
  if (m_status){
    ERROR("Issue encountered while stopping readout. Continuing tentatively...");
  }
  usleep(100);
  FaserProcess::stop();
  INFO("TRB --> readout stopped.");
}


/****************************************  
 *        Disable Trigger
 ****************************************/
void TrackerReceiverModule::disableTrigger(const std::string &) {
  m_triggerEnabled = false;
  INFO("TRB --> disable trigger.");
  usleep(100);
}

/****************************************  
 *        Enable Trigger
 ****************************************/
void TrackerReceiverModule::enableTrigger(const std::string &) {
  m_triggerEnabled = true; 
  INFO("TRB --> enable trigger.");
  usleep(100);
}

/***************************************
 *        Runner of the module
 * ************************************/
void TrackerReceiverModule::runner() noexcept {
  INFO("Running...");
  std::vector<std::vector<uint32_t>> vector_of_raw_events;
  uint8_t  local_fragment_tag = EventTags::PhysicsTag;
  uint32_t local_source_id    = SourceIDs::TrackerSourceID + m_trb->GetBoardID();
  uint64_t local_event_id;
  uint16_t local_bc_id;
  m_prev_event_id = 0;
  unsigned local_status;
  float check_point(0);

  while (m_run || vector_of_raw_events.size()) { 
    if (!m_extClkSelect && m_triggerEnabled == true){
      usleep(1e5);
      FASER::TRBAccess::GPIOCheck(m_trb.get(), &FASER::TRBAccess::GenerateL1A,m_moduleMask, false); //Generate L1A on the board
    }

    m_dataRate = m_trb->GetDataRate();
    if (std::difftime(std::time(nullptr), check_point) > m_UPDATEMETRIC_INTERVAL) { // only need to update occassionally. this sends command to TRB.
      if (m_extClkSelect && m_run) m_PLLErrCnt = m_trb->ReadPLLErrorCounter(); // ReadPLLErrorCounter is done safely. Will return 0 in case of failure.
      check_point = std::time(nullptr); 
    }

    vector_of_raw_events = m_trb->GetTRBEventData();
    m_receivedEvents = vector_of_raw_events.size();

    if (vector_of_raw_events.size() == 0){
      usleep(100); //this is to make sure we don't occupy CPU resources if no data is on output
    }
    else{
        for(std::vector<std::vector<uint32_t>>::size_type i=0; i<vector_of_raw_events.size(); i++){
          size_t total_size = vector_of_raw_events[i].size() * sizeof(uint32_t); //Event size in byte
          if (!total_size) continue;
          auto event = vector_of_raw_events[i].data();
          if ( total_size == sizeof(uint32_t) && *event == m_TRBENDOFDAQ){ // End of DAQ is a single 32 bit word.
            INFO("End of DAQ received from TRB. Expect no more incoming events.");
            continue;
          }
          m_event_size_bytes = total_size; //Monitoring data
          m_physicsEventCount += 1; //Monitoring data
         
          try { 
            TrackerDataFragment trk_data_fragment = TrackerDataFragment(event, total_size);
            local_event_id = trk_data_fragment.event_id();
            local_bc_id = (trk_data_fragment.bc_id()-m_BCIDCORR)%3564;
            local_status = 0;

            if ( trk_data_fragment.valid()) {
                auto expected_event_id = m_prev_event_id+1;
                if (local_event_id != (expected_event_id&0xffffff)){
                  WARNING("Unexpected L1 ID! Current L1 ID = "<<local_event_id<<" but was expecting "<<expected_event_id);
                  m_missedL1+=(local_event_id-(expected_event_id&0xffffff));
                  if (m_missedL1) m_status = STATUS_WARN;
                  else m_status = STATUS_OK;
                }
                m_prev_event_id = local_event_id;
            }
            else{
                local_status = EventStatus::CorruptedFragment;
                m_prev_event_id++;
                m_status=STATUS_WARN;
                m_corrupted_fragments += 1; //Monitoring data
                WARNING("Corrupted tracker data fragment at triggered event count "<<m_physicsEventCount);
                if (trk_data_fragment.has_trb_error()){
                  WARNING("TRB error found with ID "<<(int)trk_data_fragment.trb_error_id());
                }
                if (trk_data_fragment.has_module_error()){
                  for (auto mod_error : trk_data_fragment.module_error_id()){
                    WARNING("Module error found with ID "<<(int)mod_error);
                  }
                } 
                if (trk_data_fragment.has_crc_error()) {
                  WARNING("Mismatching checksums!");
                  m_checksum_mismatches++;
                  m_checksum_mismatches_rate = m_checksum_mismatches/m_physicsEventCount;
                }
            }
          } catch (TrackerDataException & e) {
              local_event_id = 0xffffff;
              local_bc_id = 0xffff;
              local_status = EventStatus::CorruptedFragment;
              m_prev_event_id++;
              m_corrupted_fragments += 1; //Monitoring data
              m_status=STATUS_WARN;
              WARNING("Crashed tracker data fragment at triggered event count "<<m_physicsEventCount);
          }

          local_event_id = local_event_id | (m_ECRcount << 24);

          std::unique_ptr<EventFragment> fragment(new EventFragment(local_fragment_tag, local_source_id, 
                                              local_event_id, local_bc_id, event, total_size));
          
          fragment->set_status(local_status);

          if (m_debug){
            DEBUG("event id: 0x"<< std::hex << fragment->event_id());
            DEBUG("fragment tag: 0x"<< std::hex << fragment->fragment_tag());
            DEBUG("source id: 0x"<< std::hex << fragment->source_id());
            DEBUG("bc id: 0x"<< std::hex << fragment->bc_id());
            DEBUG("status: 0x"<< std::hex << fragment->status());
            DEBUG("trigger bits: 0x"<< std::hex << fragment->trigger_bits());
            DEBUG("size: 0x"<< std::hex << fragment->size());
            DEBUG("payload size: 0x"<< std::hex << fragment->payload_size());
            DEBUG("timestamp: 0x"<< std::hex << fragment->timestamp());
            if (m_trace){ 
              DEBUG("Raw data received from TRB: ");
              const DAQFormats::byteVector *data = fragment->raw();
              for (unsigned i = 0; i <  (unsigned)data->size(); i++){
                  std::bitset<8> y(data->at(i));
                  DEBUG("               " << y);

              }
              delete data;
            }
          }

         // place the raw binary event fragment on the output port
         std::unique_ptr<const byteVector> bytestream(fragment->raw());
         DataFragment<daqling::utilities::Binary> binData(bytestream->data(),bytestream->size());
         m_connections.send(0, binData);

         } // event loop
       } // events retrieved
    } // while m_run

  INFO("Runner stopped");
}

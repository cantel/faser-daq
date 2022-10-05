/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#pragma once

#include "Core/DAQProcess.hpp"
#include "Commons/FaserProcess.hpp"
#include "Modules/MonitorBase/MonitorBaseModule.hpp"

/*
class TCalibMonitorModule : public FaserProcess {
 public:
  TCalibMonitorModule(const std::string& n);
  ~TCalibMonitorModule() override;

  void configure(); // optional (configuration can be handled in the constructor)
  void start(unsigned);
  void stop();

  void runner() noexcept;
};
*/

class TCalibMonitorModule : public MonitorBaseModule {
 public:
  TCalibMonitorModule(const std::string&);
  ~TCalibMonitorModule();
  void start(unsigned int);

 protected:

  void monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary);
  void register_hists( );
  void register_metrics();
  void configure( );

 private:
  uint16_t m_bcid;
  uint32_t m_l1id;
  uint16_t mapline;
  uint16_t mapline2;
  const std::string m_prefix_hname_hitp;
  const std::string m_hname_scterrors;
  
  int m_physics_trigbits;// compute avg hit metric only for given active trigger bits
  unsigned m_total_WARNINGS;
  bool m_print_WARNINGS;
  const uint8_t kSTRIPDIFFTOLERANCE = 25; // FIXME can be tuned
  const uint8_t kSTRIPS_PER_CHIP = 128;
  const uint8_t kCHIPS_PER_MODULE = 12;
  const uint8_t kTOTAL_MODULES = 8;
  const uint8_t kBCIDOFFSET = 4;
  const uint8_t kMAXWARNINGS = 50;
  static const unsigned kMAP_SIZE = 96;
  const uint16_t MAP[kMAP_SIZE][2] = {{3,18},{2,17},{3,6},{2,5},{0,18},{1,17},{0,6},{1,5},{3,19},{2,16},{3,7},{2,4},{0,19},{1,16},{0,7},{1,4},{3,20},{2,15},{3,8},{2,3},{0,20},{1,15},{0,8},{1,3},{3,21},{2,14},{3,9},{2,2},{0,21},{1,14},{0,9},{1,2},{3,22},{2,13},{3,10},{2,1},{0,22},{1,13},{0,10},{1,1},{3,23},{2,12},{3,11},{2,0},{0,23},{1,12},{0,11},{1,0},{2,23},{3,12},{2,11},{3,0},{1,23},{0,12},{1,11},{0,0},{2,22},{3,13},{2,10},{3,1},{1,22},{0,13},{1,10},{0,1},{2,21},{3,14},{2,9},{3,2},{1,21},{0,14},{1,9},{0,2},{2,20},{3,15},{2,8},{3,3},{1,20},{0,15},{1,8},{0,3},{2,19},{3,16},{2,7},{3,4},{1,19},{0,16},{1,7},{0,4},{2,18},{3,17},{2,6},{3,5},{1,18},{0,17},{1,6},{0,5}};

  std::map<int,float> m_hit_weight_assignment =  {{1,3},
                                                 {2,2},
                                                 {3,2.5},
                                                 {4,1},
                                                 {6,1.5}};

  const uint8_t kSCT_ERR_NODATA = 0x1;
  const uint8_t kSCT_ERR_BUFFOVERFLOW = 0x2;
  const uint8_t kSCT_ERR_BUFFERROR = 0x4;
  const uint8_t kSCT_ERR_UNKNOWNCHIP = 0xFF;
  int m_hits[MAXTHR][MAXMODS][NLINKS][NCHIPS][NSTRIPS];


};

//--------------------

//class HistFiller : public daqling::core::DAQProcess {

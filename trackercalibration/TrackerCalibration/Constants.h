#ifndef __Constants_h__
#define __Constants_h__

#include <string>

namespace TrackerCalib {
  
  static const int MAXTRIMRANGE = 2;   // max number of trimRanges (DAC)
  static const int MAXTRIMDAC   = 16;  // max number of trimData per trimRange (DAC)

  static const int MAXTHR       = 256; // max number of thresholds (DAC)
  static const int MAXCHARGES   = 10;  // max number of charges
  static const int MAXDELAYS    = 200; // max number of L1delays (for L1DelayScan only)
  static const int MAXSD        = 64;  // max number of Strobe-Delay values

  static const int MAXMODS      = 8;   // maximum number of modules
  static const int NLINKS       = 2;   // links / sides per module
  static const int NCHIPS       = 6;   // chips per link
  static const int NSTRIPS      = 128; // strips per chip

  //static const std::string RUN_SERVER = "localhost:5002"; // server for run service
  static const std::string RUN_SERVER = "faser-runnumber.web.cern.ch"; 
  
} // namespace TrackerCalib

#endif

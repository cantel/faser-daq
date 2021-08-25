#include "TrackerCalibration/Logger.h"
#include <fstream>

//------------------------------------------------------
TrackerCalib::Logger& TrackerCalib::Logger::instance() {
  /** Create instance (lazy initialization), to guarantee proper destruction */
  static Logger _inst;
  return _inst;
}

//------------------------------------------------------
void TrackerCalib::Logger::init(std::ostream &o1,
				std::ostream &o2){
  os1 = &o1;
  os2 = &o2;
}

//------------------------------------------------------
void TrackerCalib::Logger::extra(const std::string str,
				 int iostr){
  if(iostr == 1)
    *os1 << str;
  else if(iostr == 2)
    *os2 << str;
}


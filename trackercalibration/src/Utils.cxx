#include "TrackerCalibration/Utils.h"
#include "TrackerCalibration/Module.h"

#include <sstream>
#include <iomanip>
#include <time.h>

using namespace std;

//------------------------------------------------------
unsigned int TrackerCalib::fC2dac(float charge){
  return (unsigned int)(charge/0.0625 + 0.5);
}

//------------------------------------------------------
unsigned int TrackerCalib::mV2dac(float threshold){
  return (unsigned int)(threshold/2.5 + 0.5);
}

//------------------------------------------------------
const std::string TrackerCalib::dateStr(){
  std::time_t currTime;
  time(&currTime);
  struct tm *stm = localtime(&currTime);  
  char buffer[100];
  strftime(buffer, 50, "%d%m%y", stm);
  std::ostringstream out;
  out << buffer;
  return out.str();
}

//------------------------------------------------------
const std::string TrackerCalib::timeStr(){
  std::time_t currTime;
  time(&currTime);
  struct tm *stm = localtime(&currTime);  
  char buffer[100];
  strftime(buffer, 50, "%H%M", stm);
  std::ostringstream out;
  out << buffer;
  return out.str();
}

//------------------------------------------------------
const std::string TrackerCalib::hex2str(unsigned int val){
  std::stringstream ss;
  ss << std::hex << val;
  return ss.str();
}

//------------------------------------------------------
const std::string TrackerCalib::int2str(int val){
  std::stringstream ss;
  ss << val;
  return ss.str();
}

//------------------------------------------------------
bool TrackerCalib::isModulePresent(int trbChannel, uint8_t globalMask){
  return (globalMask & (0x1 << trbChannel));
}

//------------------------------------------------------
const std::string TrackerCalib::printTRBConfig(const FASER::ConfigReg *cfg){
  std::ostringstream out;
  out << "Content of TRB configuration register object: "<<std::endl;
  out << "Module L1AEn           = 0x" << std::hex <<(int)cfg->Module_L1En << std::endl;
  out << "Module BCREn           = 0x" << std::hex <<(int)cfg->Module_BCREn << std::endl;
  out << "Module ClkCmdSelect    = 0x" << std::hex <<(int)cfg->Module_ClkCmdSelect << std::endl;
  out << "Module Led En          = 0x" << std::hex <<(int)cfg->Led0RXEn << std::endl;
  out << "Module LedxEn          = 0x" << std::hex <<(int)cfg->Led1RXEn << std::endl;
  out << "Global config reg      = 0x" << std::hex << cfg->Global << std::endl;
  out << "      RXTimeout        = "<< std::dec << (cfg->Global & 0xff) << std::endl;
  out << "      Overflow         = "<< std::dec << ((cfg->Global >> 8) & 0xfff) << std::endl;
  out << "      L1Timeout        = "<< std::dec << ((cfg->Global >> (8+12)) & 0xff) << std::endl;
  out << "      L1TimeoutDisable = "<< std::dec << ((cfg->Global >> (8+12+8)) & 0x1) << std::endl;
  out << "      L2SoftL1AEn      = "<< std::dec << ((cfg->Global >> (8+12+8+1)) & 0x1) << std::endl;
  out << "      HardwareDelay0   = "<< std::dec << ((cfg->Global >> (8+12+8+1+1)) & 0x7) << std::endl;
  out << "      HardwareDelay1   = "<< std::dec << ((cfg->Global >> (8+12+8+1+1+3)) & 0x7) << std::endl;
  out << "      RxTimeoutDisable = "<< std::dec << ((cfg->Global >> (8+12+8+1+1+3+3)) & 0x1) << std::endl;
  out << "-----------------------------------------" << std::endl;
  return out.str();
}

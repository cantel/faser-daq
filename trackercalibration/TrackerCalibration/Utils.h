#ifndef __Utils_h__
#define __Utils_h__

#include "TrackerReadout/TRBAccess.h"
#include "TrackerReadout/TRB_ConfigRegisters.h"

#include <string>
#include <vector>

namespace TrackerCalib {

  /// forward includes from within namespace TrackerCalib
  class Module;
  
  /** Convert charge [fC] into DAC counts (calibration amplitude) */
  unsigned int fC2dac(float charge);

  /** Convert threshold [mV] into DAC counts (threshold) */
  unsigned int mV2dac(float threshold);

  /** \brief Returns string with current date.
   * 
   * Date is formatted as %d%m%y, where
   *   %d: day of the month, zero-padded (01-31)
   *   %m: month as a decimal number (01-12)
   *   %y: year, last two digits (00-99)
   */
  const std::string dateStr();

  /** \brief Returns string with current time.
   * 
   * Time is formatted as %H%M, where
   *   %H: hour in 24h format (00-23)
   *   %M: minute (00-59)
   */
  const std::string timeStr();

  /** Returns string containg hex value passed as function argument. */
  const std::string hex2str(unsigned int val);

  /** Returns string containg int value passed as function argument. */
  const std::string int2str(int val);

  /** Returns true if module defined by trbChannel is within globalMask. */
  bool isModulePresent(int trbChannel, uint8_t globalMask);

  //float ThresholdDac2mV();

  /** define some colors */
  static std::string red          = "\033[22;31m";   
  static std::string green        = "\033[22;32m"; 
  static std::string brown        = "\033[22;33m";
  static std::string blue         = "\033[22;34m"; 
  static std::string magenta      = "\033[22;35m";
  static std::string cyan         = "\033[22;36m";  
  static std::string gray         = "\033[22;37m";  
  static std::string darkgray     = "\033[30m"; 
  
  static std::string lightred     = "\033[01;31m"; 
  static std::string lightgreen   = "\033[01;32m";
  static std::string yellow       = "\033[01;33m";   
  static std::string lightblue    = "\033[01;34m"; 
  static std::string lightmagenta = "\033[01;35m";
  static std::string lightcyan    = "\033[01;36m";  
  
  static std::string bold         = "\033[1m";
  static std::string reset        = "\033[0;0m";

  /** Function to output to a string the contents of a a FASER::ConfigReg object. 
      This is basically a copy of the ConfigReg::Print() function, but needed
      to have this information stored in the output log file.
  */
  const std::string printTRBConfig(const FASER::ConfigReg*);

} // namespace TrackerCalib

#endif

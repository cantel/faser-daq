#include "TrackerCalibration/Timer.h"

#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std::chrono;

//------------------------------------------------------
TrackerCalib::Timer::Timer() :
  m_isRunning(false)
{}

//------------------------------------------------------
void TrackerCalib::Timer::start(){
  m_start = std::chrono::steady_clock::now();
  m_isRunning = true;  
}

//------------------------------------------------------
void TrackerCalib::Timer::stop(){
  m_stop = std::chrono::steady_clock::now();
  m_isRunning = false;  
}

//------------------------------------------------------
const std::string TrackerCalib::Timer::printElapsed(){

  duration<double, std::milli> elapsed = m_isRunning ? 
    (steady_clock::now() - m_start) : (m_stop - m_start);
  
  auto h = duration_cast<hours>(elapsed);   elapsed -= h;
  auto m = duration_cast<minutes>(elapsed); elapsed -= m;
  auto s = duration_cast<seconds>(elapsed); 
  
  std::ostringstream out;  
  out  << std::setfill('0')
       << std::setw(2) << h.count() << " [h] " 
       << std::setw(2) << m.count() << " [mins] " 
       << std::setw(2) << s.count() << " [secs] ";
  return out.str();
}



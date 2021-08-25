#include "TrackerCalibration/Mask.h"

#include <iostream>
#include <sstream>
#include <iomanip>

//------------------------------------------------------
TrackerCalib::Mask::Mask() :
  m_first(0),
  m_last(0),
  m_value(0x1)
{}

//------------------------------------------------------
TrackerCalib::Mask::Mask(unsigned int first,
			 unsigned int last,
			 unsigned int value) :
  m_first(first),
  m_last(last),
  m_value(value)
{}

//------------------------------------------------------
TrackerCalib::Mask::Mask(const Mask &mask) {
  if(this!=&mask){
    (*this) = mask;
  }
}

//------------------------------------------------------
TrackerCalib::Mask& TrackerCalib::Mask::operator=(const Mask &mask){
  if( this != &mask ){
    m_first = mask.m_first;
    m_last  = mask.m_last;
    m_value = mask.m_value;
  }
  return *this;
}

//------------------------------------------------------
const std::string TrackerCalib::Mask::print(int indent) {
  // white space for indent
  std::string blank="";
  for(int i=0; i<=indent; i++)
    blank += " ";
  
  // bit set representation of mask value
  std::bitset<16> bits(m_value);  

  std::ostringstream out;
  out << blank
      << "first=" << m_first
      << "  last" << m_last
      << "  value" << std::hex << m_value;
  out << " (" << bits << ")";

  return out.str();
}



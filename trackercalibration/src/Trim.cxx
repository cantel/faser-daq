#include "TrackerCalibration/Trim.h"

#include <iostream>
#include <sstream>
#include <iomanip>

//------------------------------------------------------
TrackerCalib::Trim::Trim() :
  m_channel(0),
  m_trimDac(0),
  m_trimWord(0x0)
{}

//------------------------------------------------------
TrackerCalib::Trim::Trim(unsigned int channel,
			 unsigned int trimDac) :
  m_channel(channel),
  m_trimDac(trimDac)
{
  setTrimWord();
}

//------------------------------------------------------
TrackerCalib::Trim::Trim(const Trim &trim) {
  if(this!=&trim){
    (*this) = trim;
  }
}

//------------------------------------------------------
TrackerCalib::Trim& TrackerCalib::Trim::operator=(const Trim &trim){
  if( this != &trim ){
    m_channel  = trim.m_channel;
    m_trimDac  = trim.m_trimDac;
    m_trimWord = trim.m_trimWord;
  }
  return *this;
}

//------------------------------------------------------
TrackerCalib::Trim::~Trim() {
}

//------------------------------------------------------
void TrackerCalib::Trim::setChannel(unsigned int channel) { 
  m_channel = channel; 
  setTrimWord();
}

//------------------------------------------------------
void TrackerCalib::Trim::setTrimDac(unsigned int trimDac) { 
  m_trimDac = trimDac; 
  setTrimWord();
}

//------------------------------------------------------
void TrackerCalib::Trim::setTrimData(unsigned int channel,
				     unsigned int trimDac){
  m_channel = channel;
  m_trimDac = trimDac;
  setTrimWord();
}

//------------------------------------------------------
void TrackerCalib::Trim::setTrimWord(){
  unsigned int word = m_channel & 0x007F;
  word <<= 4;
  word |= m_trimDac & 0x000F;
  m_trimWord = word;
  
}

//------------------------------------------------------
const std::string TrackerCalib::Trim::print(int indent) {
  // white space for indent
  std::string blank="";
  for(int i=0; i<=indent; i++)
    blank += " ";

  std::ostringstream out;
  return out.str();  

}



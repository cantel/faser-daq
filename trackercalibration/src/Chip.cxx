#include "TrackerCalibration/Chip.h"
#include "TrackerCalibration/Logger.h"
#include "TrackerCalibration/Utils.h"
#include "TrackerCalibration/Mask.h"
#include "TrackerCalibration/Trim.h"

#include <iostream>
#include <algorithm>
#include <iomanip>
#include <math.h>

using Trim = TrackerCalib::Trim;

//------------------------------------------------------
TrackerCalib::Chip::Chip() :
  m_address(0),
  m_cfgReg(0),
  m_biasReg(0x180d),
  m_threshcalReg(0x2810),
  m_strobeDelayReg(0x0),
  m_trimRange(0),
  m_trimTarget(0),
  m_p0(0),
  m_p1(0),
  m_p2(0){

  for(unsigned int i=0; i<128; i++)
    m_mask[i]=0;

  for(unsigned int i=0; i<4; i++)
    m_vmask[i].reserve(8);
  prepareMaskWords();

  m_trim.reserve(128);
  for(unsigned int i=0; i<128; i++)
    m_trim.push_back(new Trim(i,0));
}

//------------------------------------------------------
TrackerCalib::Chip::Chip(const Chip &chip) {
  if(this!=&chip){
    (*this) = chip;
  }
}

//------------------------------------------------------
TrackerCalib::Chip& TrackerCalib::Chip::operator=(const Chip &chip){
  if( this != &chip ){
    m_address        = chip.m_address;
    m_cfgReg         = chip.m_cfgReg;
    m_biasReg        = chip.m_biasReg;
    m_threshcalReg   = chip.m_threshcalReg;
    m_strobeDelayReg = chip.m_strobeDelayReg;
    m_trimRange      = chip.m_trimRange;
    m_trimTarget     = chip.m_trimTarget;
    m_p0             = chip.m_p0;
    m_p1             = chip.m_p1;
    m_p2             = chip.m_p2;
    m_printLevel     = chip.m_printLevel;

    for(unsigned int i=0; i<128; i++)
      m_mask[i] = chip.m_mask[i];

    for(unsigned int i=0; i<4; i++)
      m_vmask[i] = chip.m_vmask[i];

    for(auto t : m_trim)
      delete t;
    m_trim.clear();
    m_trim.reserve(128);
    for(auto t : chip.m_trim)
      m_trim.push_back(new Trim(*t));
  }
  return *this;
}

//------------------------------------------------------
TrackerCalib::Chip::~Chip() {
  for(auto t : m_trim)
    delete t;
  m_trim.clear();

  for(unsigned int i=0; i<4; i++)
    m_vmask[i].clear();
}

//------------------------------------------------------
void TrackerCalib::Chip::setCfgReg(unsigned int cfgReg) { 
  m_cfgReg = cfgReg; 

  // set trimRange
  std::bitset<16> bits(m_cfgReg);
  std::bitset<2> btr(0);
  btr.set(0,bits.test(4));
  btr.set(1,bits.test(5));
  m_trimRange = (unsigned int)btr.to_ulong();
}

//------------------------------------------------------
void TrackerCalib::Chip::setReadoutMode(ABCD_ReadoutMode mode){
  unsigned int im(0);
  switch(mode){
  case ABCD_ReadoutMode::HIT  : im=0; break;
  case ABCD_ReadoutMode::LEVEL: im=1; break;
  case ABCD_ReadoutMode::EDGE : im=2; break;
  case ABCD_ReadoutMode::TEST : im=3; break;
  default:
    break;
  }
  
  // update contents of configuration register  
  std::bitset<16> bits(m_cfgReg);
  std::bitset<2> bmode(im);
  bits.set(0,bmode.test(0));
  bits.set(1,bmode.test(1));
  m_cfgReg=(unsigned int)bits.to_ulong();  
}

//------------------------------------------------------
void TrackerCalib::Chip::setCalMode(unsigned int calmode){
  // update contents of configuration register  
  std::bitset<16> bits(m_cfgReg);
  std::bitset<2> bcm(calmode);
  bits.set(2,bcm.test(0));
  bits.set(3,bcm.test(1));
  m_cfgReg=(unsigned int)bits.to_ulong();
}

//------------------------------------------------------
void TrackerCalib::Chip::setTrimRange(unsigned int trimRange){
  m_trimRange = trimRange;

  // update contents of configuration register  
  std::bitset<16> bits(m_cfgReg);
  std::bitset<2> btr(trimRange);
  bits.set(4,btr.test(0));
  bits.set(5,btr.test(1));
  m_cfgReg=(unsigned int)bits.to_ulong();
}

//------------------------------------------------------
void TrackerCalib::Chip::setEdge(bool edge) {
  std::bitset<16> bits(m_cfgReg);
  if(edge) bits.set(6);
  else     bits.reset(6);
  m_cfgReg=(unsigned int)bits.to_ulong();
}

//------------------------------------------------------
void TrackerCalib::Chip::setMask(bool mask) {
  std::bitset<16> bits(m_cfgReg);
  if(mask) bits.set(7);
  else     bits.reset(7);
  m_cfgReg=(unsigned int)bits.to_ulong();
}

//------------------------------------------------------
void TrackerCalib::Chip::setStrobeDelay(unsigned int strobeDelay) {
  std::bitset<16> bits;
  std::bitset<6> bsd(strobeDelay);
  for(std::size_t i=0; i<bsd.size(); ++i)
    bits[i] = bsd[i];
  m_strobeDelayReg=(unsigned int)bits.to_ulong();
}

//------------------------------------------------------
void TrackerCalib::Chip::setCalAmp(float fC) {
  std::bitset<8> bdac(fC2dac(fC));
  std::bitset<16> bits(m_threshcalReg);
  for(std::size_t i=0; i<bdac.size(); ++i)
    bits[i] = bdac[i];
  m_threshcalReg=(unsigned int)bits.to_ulong();
}

//------------------------------------------------------
void TrackerCalib::Chip::setThreshold(float mV) {
  std::bitset<8> bdac(mV2dac(mV));
  std::bitset<16> bits(m_threshcalReg);
  for(std::size_t i=0; i<bdac.size(); ++i)
    bits[i+8] = bdac[i];
  m_threshcalReg=(unsigned int)bits.to_ulong();
}

//------------------------------------------------------
void TrackerCalib::Chip::setTrimDac(unsigned int channel, 
				    unsigned int trimDac){
  auto &log = TrackerCalib::Logger::instance();
  
  // sanity check
  if(channel > 127){
    log << "[Chip::setTrim] wrong channel number : " 
	<< channel << ". Do nothing..." << std::endl;
    return;
  }

  for(auto t : m_trim){
    if(t->channel() == channel){
      t->setTrimData(channel,trimDac);
      break;
    }
  }
}

//------------------------------------------------------
void TrackerCalib::Chip::setChannelMask(unsigned int channel, int mask){
  auto &log = TrackerCalib::Logger::instance();
  
  // sanity check
  if(channel > 127){
    log << "[Chip::setChannelMask] wrong channel number : " 
	      << channel << ". Do nothing... " << std::endl;
    return;
  }
  m_mask[channel] = mask;
}

//------------------------------------------------------
TrackerCalib::ABCD_ReadoutMode TrackerCalib::Chip::readoutMode() const {
  std::bitset<16> bits(m_cfgReg);
  std::bitset<2> brm(0);
  brm.set(0,bits.test(0));
  brm.set(1,bits.test(1));
  unsigned int rm = (unsigned int)brm.to_ulong();
  
  ABCD_ReadoutMode rmode;
  switch(rm){
  case 0: rmode = ABCD_ReadoutMode::HIT  ; break;
  case 1: rmode = ABCD_ReadoutMode::LEVEL; break;
  case 2: rmode = ABCD_ReadoutMode::EDGE ; break;
  case 3: rmode = ABCD_ReadoutMode::TEST ; break;
  default:
    break;
  }  
  return rmode;
}

//------------------------------------------------------
bool TrackerCalib::Chip::edge() const {
  std::bitset<16> bits(m_cfgReg);
  return bits.test(6);
}

//------------------------------------------------------
bool TrackerCalib::Chip::mask() const {
  std::bitset<16> bits(m_cfgReg);
  return bits.test(7);
}

//------------------------------------------------------
unsigned int TrackerCalib::Chip::strobeDelay() const {
  std::bitset<16> bits(m_strobeDelayReg);
  std::bitset<6> bsd;
  for(std::size_t i=0; i<bsd.size(); ++i)
    bsd[i] = bits[i];
  return (unsigned int)bsd.to_ulong();
}

//------------------------------------------------------
float TrackerCalib::Chip::calAmp() const {
  std::bitset<16> bits(m_threshcalReg);
  std::bitset<8> bcal;
  for(std::size_t i=0; i<bcal.size(); ++i)
    bcal[i] = bits[i];
  unsigned int ucal = (unsigned int)bcal.to_ulong();
  return (float)(ucal*0.0625);
}

//------------------------------------------------------
float TrackerCalib::Chip::threshold() const {
  std::bitset<16> bits(m_threshcalReg);
  std::bitset<8> bthresh;
  for(std::size_t i=0; i<bthresh.size(); ++i)
    bthresh[i] = bits[i+8];
  unsigned int uthresh = (unsigned int)bthresh.to_ulong();
  return (float)(uthresh*2.5);
}

//------------------------------------------------------
int TrackerCalib::Chip::nMasked() const {
  int tot(0);
  for(unsigned int i=0; i<128; i++)
    if( isChannelMasked(i) )
      tot++;
  return tot;
}

//------------------------------------------------------
bool TrackerCalib::Chip::isChannelMasked(unsigned int channel) const {
  auto &log = TrackerCalib::Logger::instance();
  
  // sanity check
  if(channel > 127){
    log << "[Chip::getTrimWord] wrong channel number : " 
	<< channel << ". Do nothing..." << std::endl;
    return 0;
  }
  return m_mask[channel];
}

//------------------------------------------------------
double TrackerCalib::Chip::fC2mV(double fC){
  double res = (m_p1!=0) ? m_p2+m_p0/(1+exp(-fC/m_p1)) : 0;
  return res;
}

//------------------------------------------------------
double TrackerCalib::Chip::mV2fC(double mV){
  double res(0);
  double x = mV-m_p2;
  if( x != 0 )
    res = -1*m_p1*log(m_p0/x-1);
  return res;
}

//------------------------------------------------------
void TrackerCalib::Chip::prepareMaskWords(){

  for(unsigned int ical=0; ical<4; ical++){
    // important to clear vector since we may be calling this 
    // function multiple times
    m_vmask[ical].clear();
    
    for(int i=7; i>=0; i--){
      
      // word corresponding to masking all but the channels in which
      // charge will be injected into
      uint16_t maskword = 0x1111; 	
      maskword <<= ical;
      
      uint16_t block = 0;	   
      for(int j=15; j>=0; j--){
	unsigned int istrip = 16*i+j;
	block <<= 1;
	if( m_mask[istrip] == 0 ){ // channel not to be masked
	  block |= 1;
	}
      }

      maskword &= block;
      m_vmask[ical].push_back(maskword);
    }
  } // end loop in cal-modes

}

//------------------------------------------------------
void TrackerCalib::Chip::enableAllChannelsMaskWords(){

  m_vmask[0].clear();
  
  for(int i=7; i>=0; i--){
    
    // word corresponding to enable all channels
    uint16_t maskword = 0xFFFF; 	
    
    uint16_t block = 0;	   
    for(int j=15; j>=0; j--){
      unsigned int istrip = 16*i+j;
      block <<= 1;
      if( m_mask[istrip] == 0 ){ // channel to be masked
	block |= 1;
      }
    }
    
    maskword &= block;
    m_vmask[0].push_back(maskword);
  }

}

//------------------------------------------------------
unsigned int TrackerCalib::Chip::trimDac(unsigned int channel) const {
  auto &log = TrackerCalib::Logger::instance();
  
  // sanity check
  if(channel > 127){
    log << "[Chip::getTrimWord] wrong channel number : " 
	<< channel << ". Do nothing..." << std::endl;
    return 0;
  }
  
  unsigned int trimDac(0);  
  for(auto t : m_trim){
    if(t->channel() == channel){
      trimDac = t->trimDac();
      break;
    }
  }
  return trimDac;
}

//------------------------------------------------------
unsigned int TrackerCalib::Chip::trimWord(unsigned int channel) const {
  auto &log = TrackerCalib::Logger::instance();
  
  // sanity check
  if(channel > 127){
    log << "[Chip::getTrimWord] wrong channel number : " 
	<< channel << ". Do nothing..." << std::endl;
    return 0;
  }

  unsigned int word(0);  
  for(auto t : m_trim){
    if(t->channel() == channel){
      word = t->trimWord();
      break;
    }
  }
  return word;
}

//------------------------------------------------------
const std::string TrackerCalib::Chip::print(int indent) {
  // white space for indent
  std::string blank="";
  for(int i=0; i<=indent; i++)
    blank += " ";

  // bit set representation of configuration register
  std::bitset<16> bits(m_cfgReg);  

  std::ostringstream out;
  out << blank
      << "add=" << std::dec << m_address;
  out << std::hex << " (0x" << m_address << ")";
  out << "  cfgreg=0x" << std::hex << std::setfill('0') << std::setw(4) << m_cfgReg
      << " (" << split(bits,4) << ")";
  out << "  bias=0x" << std::hex << m_biasReg;
  out << "  threg=0x" << std::hex << m_threshcalReg;
  out << "  sdreg=0x" << std::hex << std::setfill('0') << std::setw(2) << m_strobeDelayReg;
  out << "  edge=" << edge();
  out << "  mask=" << mask();
  out << "  SD=" << std::dec << std::setfill(' ') << std::setw(2) << strobeDelay()
      << "  TR=" << m_trimRange;
  out << std::fixed;
  out << "  cal[fC]=" << std::setprecision(1) << calAmp();
  out << "  th[mV]=" <<std::setprecision(0) << threshold();
  out << "  target[mV]=" << std::setprecision(1) << m_trimTarget;
  out << "  p0=" << m_p0;
  out << "  p1=" << m_p1;
  out << "  p2=" << m_p2;
  out << "  nMasked=" << nMasked();

  if( m_printLevel>1 && nMasked()!=0 ){
    out << std::endl << blank << "         --> list of masked channels: [ ";
    bool first(true);
    for(unsigned int i=0; i<128; i++)
      if( isChannelMasked(i) ){
	if( !first ) out << " , ";
	out << i;
	first=false;
      }
    out << " ]" << std::flush;
  }
  
  return out.str();
}

//------------------------------------------------------
const std::string TrackerCalib::Chip::printTrim(int indent) {
  std::string blank="";
  for(int i=0; i<=indent; i++)
    blank += " ";

  std::ostringstream out;
  for(auto t : m_trim)
    out << blank << t->print();  
  return out.str();  
}

bool IsOdd (int i) {
  return ((i%2)==1);
}


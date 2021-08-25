#include "TrackerCalibration/Module.h"
#include "TrackerCalibration/Logger.h"
#include "TrackerCalibration/Chip.h"
#include "TrackerCalibration/Utils.h"
#include "TrackerCalibration/Constants.h"

#include "TrackerReadout/ConfigurationHandling.h"

#include <iostream>
#include <iomanip>
#include <stdlib.h> /// for realpath()
#include <fstream>
#include <sstream>

using json = nlohmann::json;
using Chip = TrackerCalib::Chip;

//------------------------------------------------------
TrackerCalib::Module::Module() :
  m_id(0),
  m_planeID(0),
  m_trbChannel(0),
  m_moduleMask(0x1),
  m_jsonCfg(""),
  m_jsonCfgLast(""),
  m_printLevel(0)
{}

//------------------------------------------------------
TrackerCalib::Module::Module(std::string jsonCfg,
			     int printLevel) :
  m_id(0),
  m_planeID(0),
  m_trbChannel(0),
  m_moduleMask(0x1),
  m_jsonCfg(""),
  m_jsonCfgLast(""),
  m_printLevel(printLevel)
{
  readJson(jsonCfg);
}

//------------------------------------------------------
TrackerCalib::Module::Module(const Module &mod) {
  if(this!=&mod){
    (*this) = mod;
  }
}

//------------------------------------------------------
TrackerCalib::Module& TrackerCalib::Module::operator=(const Module &mod){
  if( this != &mod ){
    m_id          = mod.m_id;
    m_planeID     = mod.m_planeID;
    m_trbChannel  = mod.m_trbChannel;
    m_moduleMask  = mod.m_moduleMask;
    m_jsonCfg     = mod.m_jsonCfg;
    m_printLevel  = mod.m_printLevel;
    
    // clear vector of Chips
    for(auto c : m_chips)
      delete c;
    m_chips.clear();
    
    // re-populate
    m_chips.reserve(mod.m_chips.size());
    for(auto c : mod.m_chips )
      m_chips.push_back(new Chip(*c));
  }  
  return *this;
}

//------------------------------------------------------
TrackerCalib::Module::~Module() {
  for(auto c : m_chips)
    delete c;
  m_chips.clear();  
}

//------------------------------------------------------
int TrackerCalib::Module::readJson(std::string jsonCfg){
  // get logger instance
  auto &log = TrackerCalib::Logger::instance();
  
  if(m_printLevel >= 2)
    log << "[Module::readJson] for file " << jsonCfg << std::endl;

  // get full-path of configuration file
  char *fullpath = realpath(jsonCfg.c_str(),NULL);
  if( !fullpath ){
    std::string error(red+bold+"\n\nCould not find config file '"+jsonCfg+"' !! Exit.\n"+reset);
    throw std::runtime_error(error);
  }

  m_jsonCfg = std::string(fullpath);
  free(fullpath);

  // temporary HACK until get functions for ID and PlaneID become available (if needed) within FASER::ConfigHandlerSCT
  /*  std::ifstream infile(m_jsonCfg);
  json j = json::parse(infile);
  m_id = (int)j["ID"];
  m_planeID = (int)j["PlaneID"];
  infile.close();
  */
  
  // create ConfigHandlerSCT 
  FASER::ConfigHandlerSCT *handler = new FASER::ConfigHandlerSCT();
  handler->ReadFromFile(m_jsonCfg);
  m_id         = (unsigned long)handler->GetID();
  m_planeID    = (unsigned int)handler->GetPlaneID();
  m_trbChannel = (unsigned int)handler->GetTRBChannel();
  m_moduleMask <<= m_trbChannel;

  std::vector<unsigned int> chipIDs = handler->GetChipIDs();
  for(auto address : chipIDs){
    Chip *chip = new Chip();
    chip->setAddress(address);
    chip->setCfgReg(handler->GetConfigReg(address));
    chip->setBiasReg(handler->GetBiasDac(address));
    chip->setStrobeDelay(handler->GetStrobeDelay(address));
    chip->setThreshold(2.5*handler->GetThreshold(address));
    chip->setTrimTarget(handler->GetTrimTarget(address)); // already in mV
    chip->setP0(handler->GetP0(address));
    chip->setP1(handler->GetP1(address));
    chip->setP2(handler->GetP2(address)); 
    chip->setPrintLevel(m_printLevel);

    if( handler->hasStripMask(address) ){
      std::vector<uint16_t> vmask = handler->GetStripMask(address);
      //for(auto m : vmask) log << std::hex << "0x" << m << " (" <<  std::dec << m << ")" << std::endl;

      for(unsigned int i=0; i<8; i++){
	uint16_t word = vmask[i];
	for(unsigned int j=0; j<16; j++){
	  int ichan(16*i+j);
	  int val = word & (0x1 << j);
	  chip->setChannelMask(ichan,val);	  
	}
      }

      // call prepareMaskWords to update 16b words with the just read mask information
      chip->prepareMaskWords();
    }
    
    if( handler->hasTrimDAC(address) ){
      for(unsigned int i=0; i<128; i++){
	unsigned int trimdata = handler->GetTrimDac(address,i);
	unsigned int val  = trimdata & 0xF;      
	unsigned int chan = (trimdata >> 4) & 0x7F;
	chip->setTrimDac(chan,val);
	//log << "  - " << chan << " => " << val << " ( " << trimdata << " )" << std::endl;
      }
    }
    
    if( m_printLevel > 1 ){
      log << chip->print() << std::endl;
      for(unsigned int ichan=0; ichan<128; ichan++)
	log << "  - chan " << std::setw(3) << ichan << " => " 
	    << " isMasked = " << chip->isChannelMasked(ichan) << " , "
	    << " trimRange = " << chip->trimRange() << " , "
	    << " trimDac = " << std::setw(2) << chip->trimDac(ichan) 
	    << std::endl;
    }
    
    if( !addChip(chip) ){
      log << "ERROR : this module already contains a chip"
	  << " with address " << chip->address() << std::endl;
      delete chip;
      delete handler;
      return 0;
    }
  } // end loop in chipIDs

  delete handler;  

  return 1;
}


//------------------------------------------------------
int TrackerCalib::Module::writeJson(std::string outDir){

  // get logger instance
  auto &log = TrackerCalib::Logger::instance();

  if(m_printLevel >= 2)
    log << std::endl << "[Module::writeJson] nchips=" << m_chips.size() << std::endl;
  
  // create ConfigHandlerSCT
  FASER::ConfigHandlerSCT *handler = new FASER::ConfigHandlerSCT();  
  handler->SetID(m_id);
  handler->SetPlaneID(m_planeID);
  handler->SetTRBChannel(m_trbChannel);

  // fixed seed to have reproducible results when developping mask
  //srand(1234567);

  std::vector<unsigned int> vchipids;
  for( auto chip : m_chips ) 
    vchipids.push_back(chip->address());
  handler->EnableChips(vchipids);  
  vchipids.clear();
  
  for( auto chip : m_chips ){
    //if(m_printLevel > 0)
    //log << std::endl << " chip "<< chip->address() << std::endl;
    handler->SetConfigReg(chip->address(), chip->cfgReg());
    handler->SetBiasDac(chip->address(), chip->biasReg());
    handler->SetStrobeDelay(chip->address(), chip->strobeDelay());
    handler->SetThreshold(chip->address(), 0); /// TBD: change to trim target !!
    handler->SetTrimTarget(chip->address(), chip->trimTarget());
    handler->SetP0(chip->address(), chip->p0());
    handler->SetP1(chip->address(), chip->p1());
    handler->SetP2(chip->address(), chip->p2());
    for(unsigned int ichan=0; ichan<128; ichan++)
      handler->SetTrimDac(chip->address(), ichan, chip->trimDac(ichan));

    // fake mask array
    // for(unsigned int ichan=0; ichan<128; ichan++)
    //chip->setChannelMask(ichan, rand()%2);

    //int fake[6]={5,10,28,57,100,115};
    //for(int i=0; i<6; i++)
    // chip->setChannelMask(fake[i],1);
    
    // set Mask
    /*    std::vector<uint32_t> vec;
    for(unsigned int i=0; i<4; i++){
      uint32_t word(0);
      for(unsigned int j=0; j<32; j++){
	int ichan(32*i+j);	
	if( chip->isChannelMasked(ichan) ){
	  word |= (0x1 << j);
	}
      }
      vec.push_back(word);
    }
    handler->SetStripMask(chip->address(),vec);
    vec.clear();
    */

    std::vector<uint16_t> vec;
    for(unsigned int i=0; i<8; i++){
      uint16_t word(0);
      for(unsigned int j=0; j<16; j++){
	int ichan(16*i+j);	
	if( chip->isChannelMasked(ichan) ){
	  word |= (0x1 << j);
	}
      }
      vec.push_back(word);
    }
    handler->SetStripMask(chip->address(),vec);
    vec.clear();

   
    // fake TrimData
    /*for(unsigned int ichan=0; ichan<128; ichan++){
      int irand = rand() % 16;
      handler->SetTrimDac(chip->address(),ichan,irand);
      //log << "  - " << ichan << " " << irand << " " << mask[ichan] << std::endl;
    }
    */
  } // end loop in chips

  // extract json single filename from initial config file (full path)
  std::size_t pos = m_jsonCfg.find_last_of("/\\");
  std::string jsonCfg = m_jsonCfg.substr(pos+1,m_jsonCfg.length()-pos);
  
  // create output filename
  std::string filename(outDir+"/"+jsonCfg);
  filename.insert(filename.length()-5,"_"+dateStr()+"_"+timeStr());
  handler->WriteToFile(filename);

  // last json config file
  m_jsonCfgLast = filename;    
  log << "Wrote output cfg file: '" << m_jsonCfgLast << "'" << std::endl;

  delete handler;

  return 1;
}

//------------------------------------------------------
int TrackerCalib::Module::nActiveChannels(){
  int nactive = 128*m_chips.size();
  for(auto c : m_chips)
    nactive -= c->nMasked();
  return nactive;
}

//------------------------------------------------------
Chip* TrackerCalib::Module::findChip(const unsigned int address){
  Chip *chip = nullptr;
  std::vector<Chip*>::const_iterator it;
  for(it=m_chips.begin(); it!=m_chips.end(); ++it){
    if( (*it)->address() == address ){   
      chip=(*it);
      break;
    }
  }
  return chip;
}

//------------------------------------------------------
int TrackerCalib::Module::addChip(Chip *chip) {
  // check if module already contains a chip with this address
  for(auto c : m_chips){
    if( c->address() == chip->address() ){
      return 0;
    }
  }
  m_chips.push_back(chip);
  return 1;
}

//------------------------------------------------------
void TrackerCalib::Module::setPrintLevel(int printLevel) {
  m_printLevel = printLevel;
  for(auto c : m_chips)
    c->setPrintLevel(printLevel);
}

//------------------------------------------------------
const std::string TrackerCalib::Module::print(int indent){
  std::string blank="";
  if(indent!=0)
    for(int i=0; i<=indent; i++)
      blank += " ";

  std::ostringstream out;
  out << blank
      << "id=" << m_id <<  "  planeID=" << m_planeID  << "  trbChannel=" << m_trbChannel;
  
  out << "  moduleMask=0x" << std::hex << std::setfill('0')
      << std::setw(2) << unsigned(m_moduleMask);
  std::bitset<8> bits(m_moduleMask);
  std::string bstr = bits.to_string();
  out << " (" << bstr.substr(0,4) << " " << bstr.substr(4,4) << ")";
  out << std::dec << "  Nchips=" << m_chips.size()
      << "  config=" << m_jsonCfg;
  unsigned int chip_idx=0;
  if(m_printLevel > 0){
    out << reset << std::endl;
    for (auto const c : m_chips){
      out << blank << "     - chip [" << std::setw(2) << chip_idx << "]";
      out << c->print(1);
      ++chip_idx;
      if(chip_idx < m_chips.size())
	out << std::endl;
      
    }
  }
  return out.str();
}

//------------------------------------------------------
/*int TrackerCalib::Module::writeCfgFile(std::string outDir){
  auto &log = TrackerCalib::Logger::instance();

  if(m_printLevel > 0)
    log << "in [Module::writeCfgFile] ..." << std::endl;

  //
  // 1.- fill json cfg
  //
  nlohmann::json cfg;
  
  cfg["ID"]         = m_id;
  cfg["PlaneID"]    = m_planeID;
  cfg["TRBChannel"] = m_trbChannel;
  
  std::vector<json> jChips;

  for( auto chip : m_chips ){
    json jChipAtts;
    jChipAtts["Address"]        = chip->address();
    jChipAtts["BiasDAC"]        = chip->biasReg();
    jChipAtts["ConfigRegister"] = chip->cfgReg();
    jChipAtts["StrobeDelay"]    = chip->strobeDelay();
    jChipAtts["Threshold"]      = 0;

    std::vector<TrimData*> vec;
    vec.reserve(8);
    chip->trimData(vec);

    if( m_printLevel > 1 ){
      log << "- Printing trimData for chip " 
		<< std::dec << chip->address() << std::endl;
      for(auto t : vec){
	log << std::setw(3) << t->first() << " , " 
		  << std::setw(3) << t->last() << " , " 
		  << t->data() << std::endl;
      }
    }

    std::vector<json> jTrimData;    
    for(auto t : vec){
      json jTrimBlock;
      jTrimBlock["first"] = t->first();
      jTrimBlock["last"] = t->last();
      jTrimBlock["data"] = t->data();
      jTrimData.push_back(jTrimBlock);
    }
    jChipAtts["TrimData"] = jTrimData;
    jChips.push_back(jChipAtts);

    vec.clear();
    jTrimData.clear();

  } // end loop in chips
  cfg["Chips"] = jChips;

  //
  // 2.- write output file
  //
  // extract json single filename from initial config file (full path)
  std::size_t pos = m_jsonCfg.find_last_of("/\\");
  std::string jsonCfg = m_jsonCfg.substr(pos+1,m_jsonCfg.length()-pos);
  
  // create output filename
  std::string filename(outDir+"/"+jsonCfg);
  filename.insert(filename.length()-5,"_"+dateStr()+"_"+timeStr());

  std::ofstream file;
  file.open(filename);
  if ( !file.is_open() ){
    log << "ERROR: could not open output file "
	      << filename << std::endl;
    return 0;
  }
  file << std::setw(2) << cfg;
  file.close();    
  
  log << std::endl << "Wrote output cfg file: '" 
	    << filename << "'" << std::endl;
  
  return 1;
}
*/

#include "TrackerCalibration/RunManager.h"
#include "TrackerCalibration/Constants.h"
#include "TrackerCalibration/Logger.h"

#include "cpr/cpr.h"

#include <utility> 
#include <vector> 
#include <fstream> 
#include <iostream>
#include <unistd.h>

using json = nlohmann::json;

//------------------------------------------------------
TrackerCalib::RunManager::RunManager() :
  m_runNumber(-1),
  m_printLevel(0)
{}

//------------------------------------------------------
TrackerCalib::RunManager::RunManager(int printLevel) :
  m_runNumber(-1),
  m_printLevel(printLevel)
{}

//------------------------------------------------------
TrackerCalib::RunManager::~RunManager() {}

//------------------------------------------------------
int TrackerCalib::RunManager::newRun(std::string jsonCfg,
				     json &jtrbConfig,
				     json &jscanConfig) {
  auto &log = TrackerCalib::Logger::instance();

  //
  // 1. sanity check
  //
  char *rpath = realpath(jsonCfg.c_str(),NULL);
  if(rpath == nullptr){
    log << "ERROR: bad resolved path " << std::endl;
    return 0;
  }
  std::string fullpath = std::string(rpath);
  free(rpath);

  //
  // 2. parse input json file
  //
  std::ifstream infile(fullpath);
  if ( !infile.is_open() ){
    log << "[RunManager::newRun] ERROR: could not open input file " 
	<< fullpath << std::endl;
    return 0;
  } 
  json jparsedCfg = json::parse(infile);  
  infile.close();

  
  //
  // 3. Main json object for configuration
  //
  json jcfg;
  
  // json for modules configuration  
  json jmodConfig; 
  std::string modstr;

  // check if we deal with plane or single module json
  if(jparsedCfg.contains("Modules")){ // plane
    std::string modCfgFullpath;	  
    for( const auto &m : jparsedCfg["Modules"] ){
      std::string modCfg = m["cfg"];
      if( modCfg.find_last_of("/\\") == std::string::npos ){ // relative path
	bool isUnix = fullpath.find_last_of("/") != std::string::npos;
	std::size_t pos = isUnix ? fullpath.find_last_of("/") : fullpath.find_last_of("\\");	
	modCfgFullpath = fullpath.substr(0,pos+1)+modCfg;
      }
      else{
	modCfgFullpath = modCfg;
      }
      std::ifstream modfstream(modCfgFullpath);
      json jtmp = json::parse(modfstream);
      modfstream.close();
      modstr = "Module_" + std::to_string((int)jtmp["TRBChannel"]);
      jmodConfig[modstr] = jtmp;
    }
  }
  else{ // single-module
    modstr = "Module_" + std::to_string((int)jparsedCfg["TRBChannel"]);
    jmodConfig[modstr] = jparsedCfg;
  }

  jcfg["TRBConfig"]=jtrbConfig;
  jcfg["scanConfig"]=jscanConfig;
  jcfg["modConfig"]=jmodConfig;

  //
  // 4. get user
  //
  char data[255];  
  std::string user("unknown");
  if(!getlogin_r(data,255)) 
    user=std::string(data);

  //
  // create json run
  //
  json jrun;  
  jrun["type"] = "Calibration";
  jrun["configName"] = fullpath;
  jrun["version"] = "TBD";
  jrun["username"] = user;  
  jrun["startcomment"] = "tcalib";
  jrun["detectors"] = {"TBD"};
  jrun["configuration"] = jcfg;
 
  if(m_printLevel > 1 )
    log << "Dumping jrun contents : " << jrun.dump() << std::endl;

  const std::string url("http://"+RUN_SERVER+"/NewRunNumber");  
  cpr::Response r = cpr::Post(cpr::Url{url},
			      cpr::Authentication{"FASER", "HelloThere"},
			      cpr::Body{jrun.dump()},
			      cpr::Header{{"Content-Type", "application/json"}});
  if(m_printLevel > 1 ){
    std::cout << "[RunManager::newRun] status-code=" << r.status_code 
	      << " ; text=" << r.text << std::endl;
  }
  
  m_runNumber = (r.status_code != 201) ? -1 : std::stoi(r.text);
  return r.status_code;
}


//------------------------------------------------------
int TrackerCalib::RunManager::addRunInfo(json& jdata) {
  auto &log = TrackerCalib::Logger::instance();

  if(m_printLevel > 1 )
    log << "[RunManager::addRunInfo] jdata.dump = " << jdata.dump() << std::endl; 
  
  // create json object holding runinfo
  json jrun;
  jrun["runinfo"]=jdata;

  const std::string url("http://"+RUN_SERVER+"/AddRunInfo/"+std::to_string(m_runNumber));  
  cpr::Response r = cpr::Post(cpr::Url{url},
			      cpr::Authentication{"FASER", "HelloThere"},
			      cpr::Body{jrun.dump()},
			      cpr::Header{{"Content-Type", "application/json"}});
  if(m_printLevel > 1 )
    log << "[RunManager::addRunInfo] status-code=" << r.status_code 
	<< " ; text=" << r.text << std::endl;
  
  return r.status_code;
}

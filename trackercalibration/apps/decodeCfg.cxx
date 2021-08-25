#include "TrackerCalibration/Module.h"
#include "TrackerCalibration/Utils.h"

#include <iostream>
#include <stdlib.h>
#include <iomanip>
#include <string>
#include <fstream>
#include <sys/stat.h>

// input json configuration file
static std::string ifile("");

// verbosity level
static int printLevel(0);

using json = nlohmann::json;

using Module = TrackerCalib::Module;

//------------------------------------------------------
void help(){
  std::cout << std::endl;
  std::cout << "Usage: ./decodeCfg -i <JSON_FILE> " << std::endl;
  std::cout << std::endl;
  std::cout << "where : " << std::endl;
  std::cout << "  <JSON_FILE>        Input json configuration file (full plane or single module)" << std::endl;
  std::cout << std::endl;
  std::cout << "Optional arguments: " << std::endl;
  std::cout << "   -v, --verbose <VERBOSE LEVEL>   Sets the verbosity level from 0 to 3 included (default=0)." << std::endl;
  std::cout << "   -h, --help                      Displays this information and exit." << std::endl;
  std::cout << std::endl;
  exit(0);
}

//------------------------------------------------------
int getParams(int argc, char **argv) {
  int ip=1;
  int res=1;

  if(argc == 1){
    help();
  }
  else{
    while( ip < argc ){
      if(std::string(argv[ip]).substr(0,2) == "--" || std::string(argv[ip]).substr(0,1) == "-"){
	// help
	if(std::string(argv[ip])=="--help" || std::string(argv[ip])=="-h") {
	  help();
	  break;
	}
	// input file
	else if(std::string(argv[ip]) == "--ifile" || std::string(argv[ip])=="-i"){
	  if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	    ifile = argv[ip+1];
	    ip+=2; 
	  } else{
	    std::cout << std::endl << "[decodeCfg] Missing input file." << std::endl << std::endl;
	    res=0;
	    break;
	  }
	}
	// verbose level
	else if(std::string(argv[ip]) == "--verbose" || std::string(argv[ip])=="-v"){
	  if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--" && std::string(argv[ip+1]).substr(0,1) != "-") {
	    printLevel = atoi(argv[ip+1]); 
	    ip+=2; 
	  } else{
	    printLevel=1;
	    ip+=1;
	  }
	}
	else{
	  std::cout << std::endl << "[decodeCfg] Invalid argument: " << argv[1] << std::endl << std::endl;
	  res=0;
	  break;
	}
      }
      else{
	std::cout << std::endl << "[decodeCfg] Invalid argument: " << argv[1] << std::endl << std::endl;
	res=0;
	break;
      }
    }
  }
  return res;
}

//------------------------------------------------------
int readJson(std::string jsonCfg,
	     std::vector<Module*> &vec, 
	     int printLevel){

  // sanity check
  char *rpath = realpath(jsonCfg.c_str(),NULL);
  if(rpath == nullptr){
    std::cout << "[decodeCfg::readJson] ERROR: bad resolved path " << std::endl;
    return 0;
  }
  std::string fullpath = std::string(rpath);
  std::ifstream infile(fullpath);
  if ( !infile.is_open() ){
    std::cout << "[decodeCfg::readJson] ERROR: could not open input file "
	      << fullpath << std::endl;
    return 0;
  } 
  
  // parse input file 
  json j = json::parse(infile);
  infile.close();
  
  // check if we deal with a plane configuration file
  if(j.contains("Modules")){
    std::string modCfgFullpath;
	  
    for( const auto &m : j["Modules"] ){
      std::string modCfg = m["cfg"];

      /* check if it is relative or absolute path. Look for any of the two types
	 of directory separators ('/' for unix and '\' for Windows, just in case...) */
      if( modCfg.find_last_of("/\\") == std::string::npos ){ // relative path
	
	// construct full-path taking as basedir the one from the initial function argument 
	bool isUnix = fullpath.find_last_of("/") != std::string::npos;
	std::size_t pos = isUnix ? fullpath.find_last_of("/") : fullpath.find_last_of("\\");	
	modCfgFullpath = fullpath.substr(0,pos+1)+modCfg;
      }
      else{
	modCfgFullpath = modCfg;
      }
      vec.push_back(new Module(modCfgFullpath, printLevel));
    }
  }
  else{ // single module configuration file
    vec.push_back(new Module(jsonCfg, printLevel));
  }
  return 1;
}


//------------------------------------------------------
int main(int argc, char *argv[]){

  if( !getParams(argc,argv) )
    return 0;
  
  //
  // sanity checks
  //
  if( ifile.empty() ){
    std::cout << "[decodeCfg] Missing input cfg file. Exit..." << std::endl;
    return 0;
  }
  
  struct stat buffer;
  if( stat(ifile.c_str(),&buffer) == -1 ){
    std::cout << "[decodeCfg] Input cfg file '" << ifile << "' does not exist. Exit..." << std::endl;
    return 0;
  }

  //
  // read json file and fill vector of modules
  //
  std::vector<Module*> modList; 
  readJson(ifile,modList,printLevel);

  //
  // show configuration
  //
  int cnt=0;
  std::cout << "MODULES : " << modList.size() << std::endl;
  std::vector<Module*>::const_iterator mit;
  for(mit=modList.begin(); mit!=modList.end(); ++mit){
    if(printLevel > 0){
      std::cout << std::endl;
      std::cout  << TrackerCalib::bold << TrackerCalib::green;
    }
    std::cout << "   * mod [" << cnt << "] : " << (*mit)->print() 
	      << TrackerCalib::reset << std::endl;
    cnt++;
  }  
  
  return 1;
}

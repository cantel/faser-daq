#include "TrackerCalibration/Logger.h"
#include "TrackerCalibration/CalibManager.h"
#include "TrackerCalibration/Utils.h"

#include <iostream>
#include <stdlib.h>
#include <iomanip>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

using namespace std;

#ifdef __APPLE__
  #define SEDCMD "gsed"
#elif __linux__
  #define SEDCMD "sed"
#else
  #define SETCMD NULL
#endif

#define MAXBUFFER 5000

// input json configuration file
static std::string ifile("");

// output log file
static std::string logfile("log.txt");

// serie of consecutive tests
std::vector<int> testSeq;

// L1delay 
static unsigned int l1delay(130); 

// emulate TRB 
bool emulate(false);

// use CalLoop
bool doCalLoop(true);

// load Trim values in chip
bool loadTrim(true);

// create .daq file
bool saveDaq(false);

// use TRB connection via USB (ethernet otherwise)
bool usb(false);

// disable automatic creation of run number to be stored in DB
bool noRunNumber(false);

// SCIP / DAQIP address for ethernet (PC)
static std::string ipAddress("10.11.65.8");

// base output directory
static std::string outBaseDir="/home/shifter/cernbox/b161";

// verbosity level
static int printLevel(0);

//------------------------------------------------------
void help(){
  std::cout << std::endl;
  std::cout << "Usage: ./tcalib -i <JSON_FILE> -t <TEST_IDENTIFIER> [OPTION(s)] " << std::endl;
  std::cout << std::endl;
  std::cout << "where : " << std::endl;
  std::cout << "  <JSON_FILE>        Input json configuration file (full plane or single module)" << std::endl;
  std::cout << "  <TEST_IDENTIFIER>  is a number corresponding to one or more of the tests listed below:" << std::endl
	    << "     1 : L1delay scan    " << std::endl
	    << "     2 : Mask scan       " << std::endl
	    << "     3 : Threshold scan  " << std::endl
	    << "     4 : Strobe delay    " << std::endl
	    << "     5 : 3-point gain    " << std::endl
    	    << "     6 : Trim scan       " << std::endl
	    << "     7 : Response curve  " << std::endl
    	    << "     8 : Noise occupancy " << std::endl
	    << "     9 : Trigger burst   " << std::endl;
  std::cout << std::endl;
  std::cout << "Optional arguments: " << std::endl;
  //  std::cout << "   -b, --batch                     Run in batch mode, i.e, no plots shown (default=false)." << std::endl;
  std::cout << "   -o,  --outBaseDir <OUTDIR>       Base output results directory (default: " << outBaseDir << ")." << std::endl;
  std::cout << "   -v,  --verbose <VERBOSE LEVEL>   Sets the verbosity level from 0 to 3 included (default=0)." << std::endl;
  //  std::cout << "   -g, --gpioid <GPIO_ID>         GPIO address as set in hex-switch"
  //	    << " (default: " << gpioid << ")" << std::endl;
  std::cout << "   -d,  --l1delay <L1DELAY>         delay between calibration-pulse and L1A"
	    << " (default: " << l1delay << ")" << std::endl;
  std::cout << "   -l,  --log <LOG_FILE>            Output logfile, relative to outBaseDir (default: " << logfile << ")." << std::endl;
  std::cout << "   -e,  --emulate                   Emulate TRB interface (default=false)." << std::endl;
  std::cout << "   -c,  --calLoop                   Use calLoop functionality (default=true)." << std::endl;
  std::cout << "   -s,  --saveDaq                   Enable creation of .daq file (default=false)." << std::endl;
  std::cout << "   -n,  --noTrim                    Do not load in chips the single-channel trimDac values (default=false)." << std::endl;
  std::cout << "   -nr, --noRunNumber               Disable automatic creation of run-number to be stored in DB (default=false)." << std::endl;
  std::cout << "   -u,  --usb                       Use TRB connected via USB (default=false)." << std::endl;
  std::cout << "        --ip <IPADDRESS>            SCIP / DAQIP address for ethernet communication (default: " << ipAddress << ")." << std::endl;
  std::cout << "   -h,  --help                      Displays this information and exit." << std::endl;
  std::cout << std::endl;
  //std::cout << "Example: ./tcalib -i config/Plane0/Module0.json -o results -t 34 -v 2 -l log-100V.txt" << std::endl;
  //std::cout << "  will run in sequence a ThresholdScan followed by a StrobeDelay scan on module config/Plane0/Module0.json" 
  //	    << "  with verbosity level set to 2,  and output log file 'log.txt'." << std::endl;
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
      //std::cout << "Analyzing argv[" << ip << "] = " << argv[ip] << std::endl;
      
      if(std::string(argv[ip]).substr(0,2) == "--" || std::string(argv[ip]).substr(0,1) == "-"){
	// help
	if(std::string(argv[ip])=="--help" || std::string(argv[ip])=="-h") {
	  help();
	  break;
	}
	// L1delay
	else if(std::string(argv[ip]) == "--l1delay" || std::string(argv[ip])=="-d"){
	  if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	    l1delay = atoi(argv[ip+1]);
	    ip+=2; 
	  } else{
	    std::cout << std::endl << "[tcalib] Missing argument." << std::endl << std::endl;
	    res=0;
	    break;
	  }
	}
	// emulate
	else if(std::string(argv[ip])=="--emulate" || std::string(argv[ip])=="-e") {
	  emulate=true;
	  ip+=1; 
	}
	// doCalLoop
	else if(std::string(argv[ip])=="--calLoop" || std::string(argv[ip])=="-c") {
	  doCalLoop=true;
	  ip+=1; 
	}
	// saveDaq
	else if(std::string(argv[ip])=="--saveDaq" || std::string(argv[ip])=="-s") {
	  saveDaq=true;
	  ip+=1; 
	}
	// noTrim
	else if(std::string(argv[ip])=="--noTrim" || std::string(argv[ip])=="-n") {
	  loadTrim=false;
	  ip+=1; 
	}
	// noRunNumber
	else if(std::string(argv[ip])=="--noRunNumber" || std::string(argv[ip])=="-nr") {
	  noRunNumber=true;
	  ip+=1; 
	}
	// usb
	else if(std::string(argv[ip])=="--usb" || std::string(argv[ip])=="-u") {
	  usb=true;
	  ip+=1; 
	}
	// output base directory
	else if(std::string(argv[ip]) == "--odir" || std::string(argv[ip])=="-o"){
	  if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	    outBaseDir = argv[ip+1];
	    ip+=2; 
	  } else{
	    std::cout << std::endl << "[tcalib] Missing output directory." << std::endl << std::endl;
	    res=0;
	    break;
	  }
	}
	// input file
	else if(std::string(argv[ip]) == "--ifile" || std::string(argv[ip])=="-i"){
	  if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	    ifile = argv[ip+1];
	    ip+=2; 
	  } else{
	    std::cout << std::endl << "[tcalib] Missing input file." << std::endl << std::endl;
	    res=0;
	    break;
	  }
	}
	// log file
	else if(std::string(argv[ip]) == "--logfile" || std::string(argv[ip])=="-l"){
	  if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	    logfile = argv[ip+1];
	    ip+=2; 
	  } else{
	    std::cout << std::endl << "[tcalib] Missing logfile." << std::endl << std::endl;
	    res=0;
	    break;
	  }
	}
	// ip address
	else if(std::string(argv[ip]) == "--ip"){
	  if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	    ipAddress = argv[ip+1];
	    ip+=2; 
	  } else{
	    std::cout << std::endl << "[tcalib] Missing IP address parameter." << std::endl << std::endl;
	    res=0;
	    break;
	  }
	}
	// tests
	else if( std::string(argv[ip])=="-t" ){
	  if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	    std::string str(argv[ip+1]);
	    for(char& c : str) {
	      char single[3];
	      sprintf(single,"%c ",c);	      
	      int itest = atoi(single);
	       if(itest==0 || itest > 8){
		 std::cout << std::endl << "[tcalib] Invalid test identifier : " << itest << std::endl;
		 res=0;
		 break;
	       }
	       else{
		 testSeq.push_back(itest);
	       }
	    } // end loop in string characters
	    ip+=2; 
	  } else{
	    std::cout << std::endl << "[tcalib] Missing test(s) identifier." << std::endl << std::endl;
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
	  std::cout << std::endl << "Invalid argument: " << argv[1] << std::endl << std::endl;
	  res=0;
	  break;
	}
      }
      else{
	std::cout << std::endl << "Invalid argument: " << argv[1] << std::endl << std::endl;
	res=0;
	break;
      }
    }
  }
  return res;
}

//------------------------------------------------------
const char *sedCmd() {
  return (SEDCMD == NULL) ? "" : SEDCMD;
}

//------------------------------------------------------
int cleanLog(std::string outDir, std::string tmplog){
  char cmd[500];
 
  // helpful strings
  std::string fullout(outDir+"/"+logfile);
  std::string fullbase = fullout.substr(0,fullout.find_last_of("/"));
  std::string logbase=fullout.substr(fullout.find_last_of("/")+1);

  //std::cout << "fullout  = " << fullout << std::endl;
  //std::cout << "fullbase = " << fullbase << std::endl;     
  //std::cout << "logbase  = " << logbase << std::endl;
  //cout << endl;

  //
  // 0.- make sure output directory exists
  //    (might be the case if no tests are selected, since therefore
  //     ITest::setOutputDir is never called)
  //  
  struct stat buffer;  
  if( stat(fullbase.c_str(),&buffer) == -1){
    sprintf(cmd,"mkdir -p %s", fullbase.c_str());
    std::system(cmd);
  }

  //
  // 1.- check if fullout already exists.
  //     If that's the case, rename new logfile
  //
  if( realpath(fullout.c_str(),NULL) ){
    cout << "\n[tcalib] Output file '" << fullout << "' already exists !! Renaming logfile..." << endl;
    int cnt(0);
    while( realpath(fullout.c_str(),NULL) ){
      fullout = outDir+"/"+logfile+"_"+std::to_string(cnt);
      cnt++;
    }
    
    //    cout << "\n[tcalib] Output file '" << fullout << "' already exists !! "
    //	 << " => renaming log file to : " << fullout << endl;

    /*    std::vector<std::string> vfiles;
    DIR *dp=opendir(fullbase.c_str());
    struct dirent *dirp;
    while( (dirp = readdir(dp)) != NULL ){
      std::string name(dirp->d_name);
      if( name.find(logbase) != std::string::npos )
	vfiles.push_back(string(name));
    }
    closedir(dp);
    cout << "## Listing files in output directory " << endl;
    for(auto f : vfiles) cout << f << endl;

    // starting value 
    int cnt(0);
    std::string newlogbase=logbase+"_"+std::to_string(cnt);
    bool file_exists(true);
    while( file_exists ){
      for(auto f : vfiles){
	if( f.find(newlogbase) == std::string::npos){
	  file_exists=false;
	  break;
	}
      }
      cnt++;	
      if(!file_exists)
	newlogbase=logbase+"_"+std::to_string(cnt);
    }
    */
    //cout << "newlogbase = " << newlogbase << endl;
    //    std::cout << "fullbase = " << fullbase << std::endl;
    //    std::cout << "fullout  = " << fullout << std::endl;
    //    fullout = fullbase+"/"+newlogbase;
    //    vfiles.clear();    
  }

  //
  // 2.- remove ANSI color codes using sed command
  //
  sprintf(cmd,"%s -r \"s/\\x1B\\[([0-9]{1,3}(;[0-9]{1,2})?)?[mGK]//g\" %s >> %s",
    	  sedCmd(), tmplog.c_str(), fullout.c_str());
  std::system(cmd);
  
  // 
  // 3.- remove buffer file
  //
  sprintf(cmd,"rm -f %s", tmplog.c_str());
  std::system(cmd);

  
  /*  std::string logfull(outDir+"/"+logfile);
      sprintf(cmd,"mv %s %s", tmpname, logfull.c_str());
      std::system(cmd);
  */
  
  std::cout << TrackerCalib::bold
	    << "\nCreated logfile : " << fullout
	    << std::endl << std::endl;

  return 1;
}

//------------------------------------------------------
int main(int argc, char *argv[]){

  if( !getParams(argc,argv) )
    return 0;
  
  //
  // 1.- sanity checks
  //
  if( ifile.empty() ){
    std::cout << "[tcalib] Missing input cfg file. Exit..." << std::endl;
    return 0;
  }
  
  struct stat buffer;
  if( stat(ifile.c_str(),&buffer) == -1 ){
    std::cout << "[tcalib] Input cfg file does not exist. Exit..." << std::endl;
    return 0;
  }

  //
  // 2.- Logger
  //
  auto &log = TrackerCalib::Logger::instance();
  
  // create buffer file
  char ctmp[] = "/tmp/log_XXXXXXXX";
  if(mkstemp(ctmp) == -1) {
    std::cout << "Failed to create temporary file" << std::endl;
    return 0;    
  }
  std::string tmplog(ctmp);
  std::ofstream ofile(tmplog);
  log.init(std::cout,ofile);

  /* write additional information to logfile only:
     username, host, working directory and invoked command */
  char info[MAXBUFFER];
  char data[255];  
  if(!getlogin_r(data,255)){ 
    sprintf(info,"# username : %s\n", data);
    log.extra(info);
  }
  
  if(!gethostname(data,255)){
    sprintf(info,"# host     : %s\n", data);
    log.extra(info);
  }
  
  sprintf(info,"# pwd      : %s\n", getenv("PWD"));
  log.extra(info);

  std::string cmdline(argv[0]);
  for(int ip=1; ip<argc; ip++)
    cmdline += (" "+std::string(argv[ip]));
  sprintf(info,"# cmd line : %s\n", cmdline.c_str());    
  log.extra(info);

  //
  // 3.- CalibManager
  //
  TrackerCalib::CalibManager cman(outBaseDir,
				  ifile,
				  logfile,
				  testSeq,
				  l1delay,
				  emulate,
				  doCalLoop,
				  loadTrim,
				  noRunNumber,
				  usb,
				  ipAddress,
				  saveDaq,
				  printLevel);
  
  if( !cman.init() ) return 1;
  cman.run();
  cman.finalize();

  //
  // 4.- Close ofstream and do cleanup
  //
  ofile.close();
  cleanLog(cman.outBaseDir(),tmplog);  
  
  
  return 1;
}

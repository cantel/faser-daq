#include <cpr/cpr.h>
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

int main(int argc, char** argv) {

  json newRun=R"(
                { 
                  "type": "noisecalibration",
                  "configName": "threestation",
		  "version": "v1.2.0",
                  "user": "shifter",
                  "startcomment": "station 1 test",
                  "detectors": ["TRB00","TRB01","TRB02"],
		  "configuration": 
                    { "a": "b",
                      "c": [1,2,3]
		    }
                })"_json;
  
  cpr::Response r = cpr::Post(cpr::Url{"http://faser-daq-001:5002/NewRunNumber"},
			      cpr::Authentication{"FASER", "HelloThere"},
			      cpr::Body{newRun.dump()},
			      cpr::Header{{"Content-Type", "application/json"}});
  if (r.status_code != 201) {
    std::cerr << "Error [" << r.status_code << "] making request" << std::endl;
    return 1;
  }
  std::cout << "New run number: " << std::endl<<r.text << std::endl;
  int runno=std::stoi(r.text);

  json endRun=R"( { "runinfo": { "numEvents": 1000 } } )"_json;
  r = cpr::Post(cpr::Url{"http://faser-daq-001:5002/AddRunInfo/"+std::to_string(runno)},
		cpr::Authentication{"FASER", "HelloThere"},
		cpr::Body{endRun.dump()},
		cpr::Header{{"Content-Type", "application/json"}});
  if (r.status_code != 200) {
    std::cerr << "Error [" << r.status_code << "] making request" << std::endl;
    return 1;
  }
  std::cout << "UpdateStatus: "<<r.text << std::endl;

  r = cpr::Get(cpr::Url{"http://faser-daq-001:5002/RunInfo/"+std::to_string(runno)});
  if (r.status_code != 200) {
    std::cerr << "Error [" << r.status_code << "] making request" << std::endl;
    return 1;
  }
  std::cout << "Run information:"<<std::endl<<r.text << std::endl;
}

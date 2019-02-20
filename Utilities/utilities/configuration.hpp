#ifndef DAQ_UTILITIES_CONFIGURATION_HPP
#define DAQ_UTILITIES_CONFIGURATION_HPP

#include <unordered_map>
#include <vector>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>

#include "json.hpp"

#include "utilities/logging.hpp"

namespace daq
{
namespace utilities
{

class ProcessConfiguration
{

public:
  struct Values
  {
    bool a;
    bool b;
    bool c;
    bool d;
  };

  static ProcessConfiguration& instance();
//  void clear();

//  void parse_args(int argc, char** argv);
//  const Values& values();

  //nlohmann::json toJson(std::vector<std::string> path, std::vector<std::string> query);

  void load(const std::string& jsonStr);

  template <class T>
  void set(const std::string& key, const T& value); 

  std::string dump();


private:
  static std::unique_ptr<ProcessConfiguration> m_instance;
//  Values m_values;
  nlohmann::json m_config;

  ProcessConfiguration() {}



  //std::map<std::string, docopt::value> load_file(std::istream& in);
  //void store_file(std::ostream& out, std::map<std::string, docopt::value>& args, std::map<std::string, docopt::value>& defaults);
  //void set_members(std::map<std::string, docopt::value>& args);
};
}
}


#endif

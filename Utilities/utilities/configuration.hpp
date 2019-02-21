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

  void load(const std::string& jsonStr);
  std::string dump();

  template <typename T> void set(const std::string& key, const T& value) { m_config[key] = value; } 
  template <typename T> T get(const std::string& key) { return m_config[key]; }

private:
  static std::unique_ptr<ProcessConfiguration> m_instance;
  nlohmann::json m_config;

  ProcessConfiguration() {}

};

}
}


#endif

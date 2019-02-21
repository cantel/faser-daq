#ifndef DAQ_UTILITIES_CONFIGURATION_HPP
#define DAQ_UTILITIES_CONFIGURATION_HPP

#include <unordered_map>
#include <vector>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>

#include "json.hpp"

#include "utilities/Logging.hpp"
#include "utilities/Singleton.hpp"

namespace daq
{
namespace utilities
{

class ProcessConfiguration : public Singleton<ProcessConfiguration>
{

public:
  ProcessConfiguration(m_token) { INFO("ProcessConfiguration constructed."); }
  ~ProcessConfiguration() { INFO("ProcessConfiguration destructed."); }
  //static ProcessConfiguration& instance();

  void clear(){ m_config.clear(); }
  std::string dump() { return m_config.dump(); }
  void load(const std::string& jsonStr) { m_config = nlohmann::json::parse(jsonStr); }

  template <typename T> void set(const std::string& key, const T& value) { m_config[key] = value; } 
  template <typename T> T get(const std::string& key) { return m_config[key]; }

private:
//  static std::unique_ptr<ProcessConfiguration> m_instance;

  nlohmann::json m_config;

};

}
}


#endif


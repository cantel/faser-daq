#ifndef DAQ_UTILITIES_CONFIGURATION_HPP
#define DAQ_UTILITIES_CONFIGURATION_HPP

/// \cond
#include <unordered_map>
#include <vector>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
/// \endcond

#include "utilities/json.hpp"
#include "utilities/Logging.hpp"
#include "utilities/Singleton.hpp"

namespace daq
{
namespace core
{

class Configuration : public daq::utilities::Singleton<Configuration>
{

public:
  Configuration() { }
  ~Configuration() { }

  void clear(){ m_config.clear(); }
  std::string dump() { return m_config.dump(); }
  void load(const std::string& jsonStr) { m_config = nlohmann::json::parse(jsonStr); }
  nlohmann::json getConfig() { return m_config; }

  template <typename T> void set(const std::string& key, const T& value) { m_config[key] = value; } 
  template <typename T> T get(const std::string& key) { return m_config[key]; }

private:
  nlohmann::json m_config;

};

}
}

#endif


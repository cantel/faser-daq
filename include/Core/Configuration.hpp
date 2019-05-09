#ifndef DAQ_UTILITIES_CONFIGURATION_HPP
#define DAQ_UTILITIES_CONFIGURATION_HPP

/// \cond
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include "nlohmann/json.hpp"
/// \endcond

#include "Utilities/Logging.hpp"
#include "Utilities/Singleton.hpp"

namespace daq {
namespace core {

class Configuration : public daq::utilities::Singleton<Configuration> {
 public:
  Configuration() {}
  ~Configuration() {}

  void clear() { m_config.clear(); }
  std::string dump() { return m_config.dump(); }
  void load(const std::string& jsonStr) { m_config = nlohmann::json::parse(jsonStr); }
  nlohmann::json& getConfig() { return m_config; }

  template <typename T>
  void set(const std::string& key, const T& value) {
    m_config[key] = value;
  }
  template <typename T>
  T get(const std::string& key) {
    return m_config[key];
  }

 private:
  nlohmann::json m_config;
};

}  // namespace core
}  // namespace daq

#endif

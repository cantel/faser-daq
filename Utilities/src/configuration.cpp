#include <memory>
#include <iostream>
#include <fstream>
#include <regex>

#warning RS -> Version handling of config should be added.
//#include "version.h"

#include "utilities/configuration.hpp"

using namespace daq::utilities;
using namespace nlohmann;


// void felix::core::validate(boost::any& v, std::vector<std::string> const& values, hex_uint* /* target_type */, int) {
//
//     uint d;
//     try {
//         if (starts_with(s, "0x")) {
//             d = std::stoul(s.substr(2), nullptr, 16);
//         } else {
//             d = std::stoul(s, nullptr, 10);
//         }
//
//         v = boost::any(hex_uint(d));
//     } catch(std::invalid_argument) {
//         throw validation_error(validation_error::invalid_option_value);
//     }
// }


std::unique_ptr<daq::utilities::ProcessConfiguration> daq::utilities::ProcessConfiguration::m_instance;


void daq::utilities::ProcessConfiguration::load(const std::string& jsonStr) {
  m_config["version"] = 1;
  m_config["foo"] = "bar";
}

//const daq::utilities::ProcessConfiguration::Values& daq::utilities::ProcessConfiguration::values() {
//    return m_values;
//}


daq::utilities::ProcessConfiguration& daq::utilities::ProcessConfiguration::instance() {
    if(!m_instance)
        m_instance = std::unique_ptr<ProcessConfiguration>(new ProcessConfiguration());
    return *m_instance;
}

std::string daq::utilities::ProcessConfiguration::dump(){
  return m_config["foo"];
} 

/*
template <typename T>
void daq::utilities::ProcessConfiguration::set(const std::string& key, const T& value) {
  m_config[key] = value;
}

template <typename T>
T daq::utilities::ProcessConfiguration::get(const std::string& key) {
  return m_config[key];
}
*/

//void daq::utilities::ProcessConfiguration::clear() {
//    m_values = Values();
//}

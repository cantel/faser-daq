#include <memory>
#include <iostream>
#include <fstream>
#include <regex>

#warning RS -> Version handling of config should be added.
//#include "version.h"

#include "utilities/Configuration.hpp"

using namespace daq::utilities;
using namespace nlohmann;


std::unique_ptr<daq::utilities::ProcessConfiguration> daq::utilities::ProcessConfiguration::m_instance;

daq::utilities::ProcessConfiguration& daq::utilities::ProcessConfiguration::instance() {
    if(!m_instance)
        m_instance = std::unique_ptr<ProcessConfiguration>(new ProcessConfiguration());
    return *m_instance;
}


#include "utilities/Logging.hpp"
#include "core/ConnectionManager.hpp"

using namespace daq;

int
main(int argc, char** argv)
{

  INFO("WOOF WOOF");
  WARNING("Ugh!" << 12345 << "bof bof" << '\n');

  INFO("Testing ConnectionManager.hpp");
  daq::core::ConnectionManager& cm = daq::core::ConnectionManager::instance();
  

  ERROR("About to die...");
  return 0;
}


#include "Utilities/Logging.hpp"
#include "Core/ConnectionManager.hpp"

using namespace daq;

int
main(int argc, char** argv)
{

  INFO("WOOF WOOF");
  WARNING("Ugh!" << 12345 << "bof bof" << '\n');

  INFO("Testing ConnectionManager.hpp");
  daq::core::ConnectionManager<int, int>& cm = daq::core::ConnectionManager<int, int>::instance();
  

  ERROR("About to die...");
  return 0;
}


#include "utilities/Logging.hpp"

int
main(int argc, char** argv)
{

  INFO("WOOF WOOF");
  WARNING("Ugh!" << 12345 << "bof bof" << '\n');
  ERROR("About to die...");
  return 0;
}


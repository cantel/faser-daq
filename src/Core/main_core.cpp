/// \cond
#include <cstdlib>
/// \endcond

#include "Core/Core.hpp"

using namespace std::chrono_literals;

int main(int argc, char **argv) {
  if (argc == 1) {
    ERROR("No command port provided!");
    return 1;
  }
  int port = atoi(argv[1]);
  INFO("Port " << port);
  daq::core::Core c(port, "tcp", "*");

  c.setupCommandPath();
  c.setupCommandHandler();

  std::mutex *mtx = c.getMutex();
  std::condition_variable *cv = c.getCondVar();

  std::unique_lock<std::mutex> lk(*mtx);

  cv->wait(lk, [&] {
    INFO("Checking condition " << c.getShouldStop());
    return c.getShouldStop();
  });
  lk.unlock();
  INFO("Condition met!");

  return 0;
}

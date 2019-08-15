
#pragma once

/// \cond
#include <string>
/// \endcond

#include "../Udp.hpp"
#include "Core/DAQProcess.hpp"

class FrontEndReceiverModule : public daqling::core::DAQProcess {
 public:
  FrontEndReceiverModule();
  ~FrontEndReceiverModule();
  void start();
  void stop();

  void runner();
private:
  UdpReceiver m_dataIn;
  std::atomic<int> m_recvCount;
};

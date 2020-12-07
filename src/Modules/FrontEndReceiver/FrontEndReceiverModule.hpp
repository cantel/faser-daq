
#pragma once

/// \cond
#include <string>
/// \endcond

#include "Utils/Udp.hpp"
#include "Commons/FaserProcess.hpp"

class FrontEndReceiverModule : public FaserProcess {
 public:
  FrontEndReceiverModule();
  ~FrontEndReceiverModule();
  void configure();
  void sendECR();
  void start(unsigned int);
  void stop();

  void runner() noexcept;
private:
  UdpReceiver m_dataIn;
  std::atomic<int> m_recvCount;
};

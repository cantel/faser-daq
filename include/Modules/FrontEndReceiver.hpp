
#ifndef FRONTENDRECEIVER_HPP_
#define FRONTENDRECEIVER_HPP_

/// \cond
#include <string>
/// \endcond

#include "Utilities/Udp.hpp"
#include "Core/DAQProcess.hpp"

class FrontEndReceiver : public daqling::core::DAQProcess {
 public:
  FrontEndReceiver(std::string name, int num);
  ~FrontEndReceiver();
  void start();
  void stop();

  void runner();
private:
  UdpReceiver m_dataIn;
  std::atomic<int> m_recvCount;
};

#endif /* FRONTENDRECEIVER_HPP_ */

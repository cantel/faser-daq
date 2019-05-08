#ifndef FRONTENDEMULATOR_H_
#define FRONTENDEMULATOR_H_

#include "Utilities/Udp.hpp"
#include "Core/DAQProcess.hpp"

class FrontEndEmulator : public DAQProcess {
 public:
  FrontEndEmulator();
  ~FrontEndEmulator();

  void start();
  void stop();

  void runner();
private:
  int m_meanSize;
  int m_rmsSize;
  int m_fragID;
  UdpReceiver m_trigIn;
  UdpSender m_outHandle;
  int m_eventCounter;
};

#endif /* FRONTENDEMULATOR_H_ */

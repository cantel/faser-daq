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
  uint32_t m_fragID;
  float m_probMissTrig;
  float m_probMissFrag;
  float m_probCorruptFrag;
  UdpReceiver m_trigIn;
  UdpSender m_outHandle;
  int m_eventCounter;
};

#endif /* FRONTENDEMULATOR_H_ */

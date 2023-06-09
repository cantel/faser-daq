/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#pragma once

/// \cond
#include <string>
/// \endcond

#include "Utils/Udp.hpp"
#include "Commons/FaserProcess.hpp"

class FrontEndReceiverModule : public FaserProcess {
 public:
  FrontEndReceiverModule(const std::string&);
  ~FrontEndReceiverModule();
  void configure();
  void sendECR();
  void start(unsigned int);
  void stop();

  void runner() noexcept;
private:
  UdpReceiver m_dataIn;
  std::atomic<int> m_recvCount;
  std::atomic<int> m_physicsCount;
  std::atomic<int> m_monitoringCount;
};

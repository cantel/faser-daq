#ifndef DAQ_MODULES_FILEDATALOGGER_HPP_
#define DAQ_MODULES_FILEDATALOGGER_HPP_

/// \cond
#include <iostream>
#include <fstream>
#include <queue>
#include <map>
#include <random>
/// \endcond

#include "Core/DAQProcess.hpp"
#include "Core/DataLogger.hpp"
#include "Utilities/Binary.hpp"
#include "Utilities/ChunkedStorage.hpp"
#include "Utilities/ProducerConsumerQueue.hpp"


/*
 * FileDataLogger
 * Author: Roland.Sipos@cern.ch
 * Description: Data logger for binary files with fstream.
 *   Relies on fixed size file IO with Binary splitting and concatenation.
 * Date: April 2019
 */
class FileDataLogger : public DAQProcess, public DataLogger {
public:
  FileDataLogger();
  ~FileDataLogger();

  void start();
  void stop();
  void runner();

  void setup();
  void write();
  void read();
  bool write(uint64_t keyId, daq::utilities::Binary& payload);
  void shutdown();

private:
  // Configs
  long m_writeBytes;

  // Internals 
  folly::ProducerConsumerQueue<daq::utilities::Binary> m_payloads;
  daq::utilities::Binary m_buffer;
  std::map<uint64_t, std::unique_ptr<daq::utilities::ReusableThread>> m_fileWriters;
  std::map<uint64_t, std::function<void()>> m_writeFunctors;
  std::map<uint64_t, std::string> m_fileNames; 
  std::map<uint64_t, std::fstream> m_fileStreams;
  std::map<uint64_t, daq::utilities::Binary> m_fileBuffers;
  std::map<uint64_t, uint32_t> m_fileRotationCounters; 

  // Random for testing.
  std::random_device m_randDevice;
  std::mt19937 m_mt;
  std::uniform_int_distribution<int> m_uniformDist;
  std::string m_dummyStr;

  // Thread control
  std::atomic<bool> m_stopWriters;

};

#endif /* DAQ_MODULES_FILEDATALOGGER_HPP_ */

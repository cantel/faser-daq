/**
 * Copyright (C) 2019 CERN
 *
 * DAQling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DAQling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with DAQling. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DAQLING_MODULES_DISRUPTORFILEDATALOGGER_HPP
#define DAQLING_MODULES_DISRUPTORFILEDATALOGGER_HPP

/// \cond
#include <fstream>
#include <map>
#include <queue>
/// \endcond

#include "Core/DAQProcess.hpp"
#include "Core/DataLogger.hpp"
#include "Utilities/Binary.hpp"
#include "Utilities/ChunkedStorage.hpp"
#include "Utilities/ProducerConsumerQueue.hpp"

/*
 * FileDataLogger
 * Description: Data logger for binary files with fstream.
 *   Relies on fixed size file IO with Binary splitting and concatenation.
 * Date: April 2019
 */
class DisruptorFileDataLogger : public daqling::core::DAQProcess, public daqling::core::DataLogger
{
 public:
  DisruptorFileDataLogger();
  ~DisruptorFileDataLogger();

  void start();
  void stop();
  void runner();

  void monitor_runner();

  void setup();
  void write();
  void read();
  void writeToFile(int ftid);
  bool write(uint64_t keyId, daqling::utilities::Binary& payload);
  void shutdown();

 private:
  // Configs
  long m_writeBytes;

  // Internals
  folly::ProducerConsumerQueue<daqling::utilities::Binary> m_payloads;
  daqling::utilities::Binary m_buffer;
  std::map<uint64_t, std::unique_ptr<daqling::utilities::ReusableThread>> m_fileWriters;
  std::map<uint64_t, std::function<void()>> m_writeFunctors;
  std::map<uint64_t, std::string> m_fileNames;
  std::map<uint64_t, std::fstream> m_fileStreams;
  std::map<uint64_t, std::vector<daqling::utilities::Binary>> m_fileBuffers;
  std::map<uint64_t, uint32_t> m_fileRotationCounters;

  //write thread
  std::thread fileWriter;

  std::atomic<int> m_bytes_sent;
  std::unique_ptr<std::thread> m_monitor_thread;
  // Thread control
  std::atomic<int> m_bufferReady; //Indicates which 4K buffer is ready to be written from the vector.
  std::atomic<bool> m_stopWriters;
};

#endif  // DAQLING_MODULES_DISRUPTORFILEDATALOGGER_HPP


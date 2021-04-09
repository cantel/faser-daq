/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#pragma once

/// \cond
#include <condition_variable>
#include <fstream>
#include <map>
#include <memory>
#include <queue>
#include <tuple>
/// \endcond

#include "Commons/FaserProcess.hpp"
#include "Utils/Binary.hpp"
#include "Utils/ProducerConsumerQueue.hpp"
#include "Utils/Ers.hpp"


/**
 * Issues related to FileWriterFaserModule.
 */
ERS_DECLARE_ISSUE(FileWriterIssues,                                                             // Namespace
                  TimestampFormatFailed,                                                   // Class name
                  "Failed to format timestamp", // Message
                  ERS_EMPTY)                      // Args

ERS_DECLARE_ISSUE(FileWriterIssues,                                                             // Namespace
                  UnknownOutputFileArgument,                                                   // Class name
                  "Unknown output file argument '" << c << "'", // Message
                  ((const char *)c))                      // Args

ERS_DECLARE_ISSUE(FileWriterIssues,                                                             // Namespace
                  MissingChannelNames,                                                   // Class name
                  "Missing channel names", // Message
                  ERS_EMPTY)                      // Args

ERS_DECLARE_ISSUE(FileWriterIssues,                                                             // Namespace
                  InvalidFileNamePattern,                                                   // Class name
                  "Invalid file name pattern: " << c, // Message
                  ((const char *)c))                      // Args

ERS_DECLARE_ISSUE(FileWriterIssues,                                                             // Namespace
                  OfstreamFailed,                                                   // Class name
                  " Write operation for channel " << chid << " of size " << size
                                               << "B failed!", // Message
                  ((uint_64t)chid)
                  ((size_t)size))                      // Args
/**
 * Module for writing your acquired data to file.
 */
class FileWriterFaserModule : public FaserProcess {
public:
  FileWriterFaserModule();
  ~FileWriterFaserModule();

  void configure();
  void start(unsigned run_num);
  void stop();
  void runner() noexcept;

  void monitor_runner();

private:
  struct ThreadContext {
    ThreadContext(std::array<unsigned int, 2> tids) : consumer(tids[0]), producer(tids[1]) {}
    daqling::utilities::ReusableThread consumer;
    daqling::utilities::ReusableThread producer;
  };
  using PayloadQueue = folly::ProducerConsumerQueue<daqling::utilities::Binary>;
  using Context = std::tuple<PayloadQueue, ThreadContext>;

  struct Metrics {
    std::atomic<size_t> bytes_written = 0;
    std::atomic<size_t> payload_queue_size = 0;
    std::atomic<size_t> payload_size = 0;
  };

  size_t m_buffer_size;
  std::string m_pattern;
  std::map<int,std::string> m_channel_names;
  std::atomic<bool> m_start_completed;

  /**
   * Output file generator with a printf-like filename pattern.
   */
  class FileGenerator {
  public:
    FileGenerator(const std::string pattern, const std::string channel_name, const uint64_t chid, const unsigned run_number)
      : m_pattern(pattern), m_channel_name(channel_name), m_chid(chid), m_run_number(run_number) {}

    /**
     * Generates the next output file in the sequence.
     *
     * @warning Silently overwrites files if they already exists
     * @warning Silently overwrites previous output files if specified pattern does not generate
     * unique file names.
     */
    std::ofstream next();

    /**
     * Returns whether `pattern` yields unique output files on rotation.
     * Effectively checks whether the pattern contains %n.
     */
    static bool yields_unique(const std::string &pattern);

  private:
    const std::string m_pattern;
    const std::string m_channel_name;
    const uint64_t m_chid;
    unsigned m_filenum = 0;
    const unsigned m_run_number;
  };

  // Configs
  size_t m_max_filesize;
  uint64_t m_channels = 0;

  // Thread control
  std::atomic<bool> m_stopWriters;

  // Metrics
  mutable std::map<uint64_t, Metrics> m_channelMetrics;

  // Internals
  void flusher(const uint64_t chid, PayloadQueue &pq, const size_t max_buffer_size,
               FileGenerator fg) const;
  std::map<uint64_t, Context> m_channelContexts;
  std::thread m_monitor_thread;
};

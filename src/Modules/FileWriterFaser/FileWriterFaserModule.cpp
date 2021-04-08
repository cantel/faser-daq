/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

/// \cond
#include <chrono>
#include <ctime>
#include <sstream>
/// \endcond

#include "FileWriterFaserModule.hpp"
#include "Utils/Logging.hpp"

using namespace std::chrono_literals;
namespace daqutils = daqling::utilities;

static std::string to_zero_lead(const int value, const unsigned precision)
{
     std::ostringstream oss;
     oss << std::setw(precision) << std::setfill('0') << value;
     return oss.str();
}

std::ofstream FileWriterFaserModule::FileGenerator::next() {

  const auto handle_arg = [this](char c) -> std::string {
    switch (c) {
    case 'D': // Full date in YYYY-MM-DD-HH:MM:SS (ISO 8601) format
    {
      std::time_t t = std::time(nullptr);
      char tstr[32];
      if (!std::strftime(tstr, sizeof(tstr), "%F-%T", std::localtime(&t))) {
        throw std::runtime_error("Failed to format timestamp");
      }
      return std::string(tstr);
    }
    case 'n': // The nth generated output (equals the number of times called `next()`, minus 1)
      return to_zero_lead(m_filenum++,5);
    case 'c': // The channel id
      return m_channel_name;
    case 'r': // The run number
    return to_zero_lead(m_run_number,6);
    default:
      std::stringstream ss;
      ss << "Unknown output file argument '" << c << "'";
      throw std::runtime_error(ss.str());
    }
  };

  // Append every character until we hit a '%' (control character),
  // where the next character denotes an argument.
  std::stringstream ss;
  for (auto c = m_pattern.cbegin(); c != m_pattern.cend(); c++) {
    if (*c == '%' && c + 1 != m_pattern.cend()) {
      ss << handle_arg(*(++c));
    } else {
      ss << *c;
    }
  }

  DEBUG("Next generated filename is: " << ss.str());

  return std::ofstream(ss.str(), std::ios::binary);
}

bool FileWriterFaserModule::FileGenerator::yields_unique(const std::string &pattern) {
  std::map<char, bool> fields{{'n', false}, {'c', false}, {'r', false}};

  for (auto c = pattern.cbegin(); c != pattern.cend(); c++) {
    if (*c == '%' && c + 1 != pattern.cend()) {
      try {
        fields.at(*(++c)) = true;
      } catch (const std::out_of_range &) {
        continue;
      }
    }
  }

  /*
   * Just to be sure, make sure every field is specified except date.
   */
  return std::all_of(fields.cbegin(), fields.cend(), [](const auto &f) { return f.second; });
}

FileWriterFaserModule::FileWriterFaserModule() : m_stopWriters{false} {
  DEBUG("");

  // Set up static resources...
  std::ios_base::sync_with_stdio(false);
}

FileWriterFaserModule::~FileWriterFaserModule() { DEBUG(""); }

void FileWriterFaserModule::configure() {
  FaserProcess::configure();
  // Read out required and optional configurations
  m_max_filesize = m_config.getSettings().value("max_filesize", 1 * daqutils::Constant::Giga);
  m_buffer_size = m_config.getSettings().value("buffer_size", 4 * daqutils::Constant::Kilo);
  m_stop_timeout = m_config.getSettings().value("stop_timeout_ms", 1500);
  uint64_t ch=0;
  for ( auto& name : m_config.getSettings()["channel_names"]) {
    m_channel_names[ch]=name;
    INFO("Channel "<<ch<<": "<<name);
    ch++;
  }
  m_channels = m_config.getConnections()["receivers"].size();
  if (ch<m_channels) {
    CRITICAL("Channel names needs to be supplied for all input channels");
    throw std::logic_error("Missing channel names");
  }
  m_pattern = m_config.getSettings()["filename_pattern"];
  INFO("Configuration:");
  INFO(" -> Maximum filesize: " << m_max_filesize << "B");
  INFO(" -> Buffer size: " << m_buffer_size << "B");
  INFO(" -> channels: " << m_channels);

  if (!FileGenerator::yields_unique(m_pattern)) {
    CRITICAL("Configured file name pattern '"
             << m_pattern
             << "' may not yield unique output file on rotation; your files may be silently "
                "overwritten. Ensure the pattern contains all fields ('%c', '%n' and '%D').");
    throw std::logic_error("invalid file name pattern");
  }

  DEBUG("setup finished");

  // Contruct variables for metrics
  for (uint64_t chid = 0; chid < m_channels; chid++) {
    m_channelMetrics[chid];
  }

  if (m_statistics) {
    // Register statistical variables
    for (auto & [ chid, metrics ] : m_channelMetrics) {
      m_statistics->registerMetric<std::atomic<size_t>>(&metrics.bytes_written,
                                                        fmt::format("BytesWritten_{}",  m_channel_names[chid]),
                                                        daqling::core::metrics::RATE);
      m_statistics->registerMetric<std::atomic<size_t>>(
          &metrics.events_received, fmt::format("EventsReceived_{}", m_channel_names[chid]),
          daqling::core::metrics::LAST_VALUE);
      m_statistics->registerMetric<std::atomic<size_t>>(
          &metrics.files_written, fmt::format("FilesWritten_{}", m_channel_names[chid]),
          daqling::core::metrics::LAST_VALUE);
      m_statistics->registerMetric<std::atomic<size_t>>(
          &metrics.payload_queue_size, fmt::format("PayloadQueueSize_{}", m_channel_names[chid]),
          daqling::core::metrics::LAST_VALUE);
      m_statistics->registerMetric<std::atomic<size_t>>(&metrics.payload_size,
                                                        fmt::format("PayloadSize_{}", m_channel_names[chid]),
                                                        daqling::core::metrics::AVERAGE);
    }
    DEBUG("Metrics are setup");
  }
}

void FileWriterFaserModule::start(unsigned run_num) {
  m_start_completed.store(false);
  FaserProcess::start(run_num);

  m_stopWriters.store(false);
  unsigned int threadid = 11111;       // XXX: magic
  constexpr size_t queue_size = 10000; // XXX: magic

  for (uint64_t chid = 0; chid < m_channels; chid++) {
    // For each channel, construct a context of a payload queue, a consumer thread, and a producer
    // thread.
    std::array<unsigned int, 2> tids = {threadid++, threadid++};
    const auto & [ it, success ] =
        m_channelContexts.emplace(chid, std::forward_as_tuple(queue_size, std::move(tids)));
    assert(success);

    // Start the context's consumer thread.
    std::get<ThreadContext>(it->second)
        .consumer.set_work(&FileWriterFaserModule::flusher, this, it->first,
                           std::ref(std::get<PayloadQueue>(it->second)), m_buffer_size,
                           FileGenerator(m_pattern, m_channel_names[chid], it->first, m_run_number));
  }
  assert(m_channelContexts.size() == m_channels);

  m_monitor_thread = std::thread(&FileWriterFaserModule::monitor_runner, this);

  m_start_completed.store(true);
}

void FileWriterFaserModule::stop() {

  std::this_thread::sleep_for(std::chrono::milliseconds(m_stop_timeout)); //FIXME: temporary until we have ordered transitions

  FaserProcess::stop();
  m_stopWriters.store(true);
  for (auto & [ chid, ctx ] : m_channelContexts) {
    while (!std::get<ThreadContext>(ctx).consumer.get_readiness()) {
      std::this_thread::sleep_for(1ms);
    }
  }
  m_channelContexts.clear();

  if (m_monitor_thread.joinable()) {
    m_monitor_thread.join();
  }
}

void FileWriterFaserModule::runner() noexcept {
  DEBUG(" Running...");

  while (!m_start_completed) {
    std::this_thread::sleep_for(1ms);
  }

  // Start the producer thread of each context
  for (auto & [ chid, ctx ] : m_channelContexts) {
    std::get<ThreadContext>(ctx).producer.set_work([&]() {
      auto &pq = std::get<PayloadQueue>(ctx);

      while (m_run) {
        daqutils::Binary pl;
        while (!m_connections.receive(chid, std::ref(pl)) && m_run) {
          if (m_statistics) {
            m_channelMetrics.at(chid).payload_queue_size = pq.sizeGuess();
          }
          std::this_thread::sleep_for(1ms);
        }

        DEBUG(" Received " << pl.size() << "B payload on channel: " << chid);
        while (!pq.write(pl) && m_run)
          ; // try until successful append
        if (m_statistics && pl.size()) {
          m_channelMetrics.at(chid).payload_size = pl.size();
          m_channelMetrics.at(chid).events_received++;
        }
      }
    });
  }

  while (m_run) {
    std::this_thread::sleep_for(1ms);
  };

  DEBUG(" Runner stopped");
}

void FileWriterFaserModule::flusher(const uint64_t chid, PayloadQueue &pq, const size_t max_buffer_size,
                               FileGenerator fg) const {
  size_t bytes_written = 0;
  std::ofstream out = fg.next();
  auto buffer = daqutils::Binary();
  m_channelMetrics.at(chid).files_written = 1;
  const auto flush = [&](daqutils::Binary &data) {
    out.write(data.data<char *>(), static_cast<std::streamsize>(data.size()));
    if (out.fail()) {
      CRITICAL(" Write operation for channel " << chid << " of size " << data.size()
                                               << "B failed!");
      throw std::runtime_error("std::ofstream::fail()");
    }
    m_channelMetrics.at(chid).bytes_written += data.size();
    bytes_written += data.size();
    data = daqutils::Binary();
  };

  while (!m_stopWriters) {
    while (pq.isEmpty() && !m_stopWriters) { // wait until we have something to write
      std::this_thread::sleep_for(1ms);
    };
    if (m_stopWriters) {
      flush(buffer);
      return;
    }

    if (bytes_written + buffer.size() > m_max_filesize) { // Rotate output files
      INFO(" Rotating output files for channel " << chid);
      m_channelMetrics.at(chid).files_written++;
      flush(buffer);
      out.flush();
      out.close();
      out = fg.next();
      bytes_written = 0;
    }

    auto payload = pq.frontPtr();

    if (payload->size() + buffer.size() <= max_buffer_size) {
      buffer += *payload;
    } else {
      DEBUG("Processing buffer split.");
      const size_t split_offset = max_buffer_size - buffer.size();
      size_t tail_len = buffer.size() + payload->size() - max_buffer_size;
      assert(tail_len > 0);

      // Split the payload into a head and a tail
      daqutils::Binary head(payload->data(), split_offset);
      daqutils::Binary tail(payload->data<char *>() + split_offset, tail_len);
      DEBUG(" -> head length: " << head.size() << "; tail length: " << tail.size());
      assert(head.size() + tail.size() == payload->size());

      buffer += head;
      flush(buffer);

      // Flush the tail until it is small enough to fit in the buffer
      while (tail_len > max_buffer_size) {
        daqutils::Binary body(tail.data(), max_buffer_size);
        daqutils::Binary next_tail(tail.data<char *>() + max_buffer_size,
                                   tail_len - max_buffer_size);
        assert(body.size() + next_tail.size() == tail.size());
        flush(body);

        tail = next_tail;
        tail_len = next_tail.size();
        DEBUG(" -> head of tail flushed; new tail length: " << tail_len);
      }

      buffer = std::move(tail);
      assert(buffer.size() <= max_buffer_size);
    }

    // We are done with the payload; destruct it.
    pq.popFront();
  }
}

void FileWriterFaserModule::monitor_runner() {
  std::map<uint64_t, unsigned long> prev_value;
  while (m_run) {
    std::this_thread::sleep_for(1s);
    for (auto & [ chid, metrics ] : m_channelMetrics) {
      DEBUG("Bytes written (channel "
           << chid
           << "): " << static_cast<double>(metrics.bytes_written - prev_value[chid]) / 1000000
           << " MBytes/s");
      prev_value[chid] = metrics.bytes_written;
    }
  }
}

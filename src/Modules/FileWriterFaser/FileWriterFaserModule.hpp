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
#include <utility>
#include <fstream>
#include <map>
#include <queue>
/// \endcond

#include "Core/DAQProcess.hpp"
#include "Core/DataLogger.hpp"
#include "Utilities/Binary.hpp"
#include "Utilities/ChunkedStorage.hpp"
#include "Utilities/ProducerConsumerQueue.hpp"
#include "Modules/EventFormat.hpp"

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

    /** 
    In start we spawn the monitor, file writer and book keeper threads.
    Inputs: none.
    Output: none.
    */
    void start();

    void stop();

    /** 
    In runner we get the binary payloads and insert them into the producer consumer buffer (m_payloads). In addition we call the bad event handler
    incase of a corrupted event.
    Inputs: none.
    Output: none.
    */
    void runner();

    void monitor_runner();

    /** 
    In setup we initialize our write functors. The write functors are responsible for filling m_fileBuffers with data right before it's
    written into a file. The write functors also signal to the file writer that the data is ready.
    Inputs: none.
    Output: none.
    */
    void setup();
    
    //write and read are not used but are pure virtual in DataLogger so we supply a dummy implementation.
    void write();
    
    void read();
    
    /**
    This is our write method. We write to different files based on the event we got. The splitting is done according to the event tag.
    the event into
    Inputs: int ftid - less relevant for the current version, but it indicates the channel we get data from (needed if we have multiple event builders)
    Output: Files with the events information. Currently the files are stored under daq/build.
    */
    void writeToFile(int ftid);

    /**
    Book keeper is responsible for the file rotation, basically opening and closing the different files that we write
    the event into
    Inputs: int ftid - less relevant for the current version, but it indicates the channel we get data from (needed if we have multiple event builders)
    Output: none
    */
    void bookKeeper(int ftid);

    /**
    This method is responsible for dumping data of corrupted events (Corrupted event - an event with a tag that wasn't specified in the config file)
    into a file specified in the config.
    Inputs: const EventHeader* badEvent - Pointer to the bad event header.
    Output: File with the corrupted events information. Currently the file is stored under daq/build.
    */
    void handleBadEvent(const EventHeader* badEvent);

    //Write is a pure virtual function in DataLogger so we have to override it. We don't use it though.
    bool write(uint64_t keyId, daqling::utilities::Binary& payload);
    
    void shutdown();

  private:
    // Configs
    long m_writeBytes;
    long long m_maxFileSize;

    // Internals
    folly::ProducerConsumerQueue<daqling::utilities::Binary> m_payloads;
    daqling::utilities::Binary m_buffer;
    std::map<uint64_t, std::unique_ptr<daqling::utilities::ReusableThread>> m_fileWriters;
    std::map<uint64_t, std::function<void()>> m_writeFunctors;
    std::map<uint8_t, std::string> m_fileNames;
    std::map<uint8_t, std::string> m_fileNameFormats;

    std::map<uint64_t, std::map<uint8_t, std::pair<std::fstream, std::fstream>>> m_fileStreams;

    std::map<uint8_t, std::vector<daqling::utilities::Binary>> m_fileBuffers;
    std::map<uint8_t, uint32_t> m_fileRotationCounters;

    uint8_t m_eventTag;
    std::map<uint8_t, uint32_t> m_fileStreamsReady;

    std::ofstream m_badEventsDumpFile;

    //write thread
    std::unique_ptr<std::thread> m_fileWriter;
    std::unique_ptr<std::thread> m_bookKeeper;

    std::atomic<int> m_bytes_sent;
    std::unique_ptr<std::thread> m_monitor_thread;
    // Thread control
    std::atomic<int> m_bufferReady; //Indicates which 24K buffer is ready to be written from the vector.
    std::atomic<int> m_fstreamFull;
    std::atomic<bool> m_stopWriters;
};

#endif  // DAQLING_MODULES_DISRUPTORFILEDATALOGGER_HPP


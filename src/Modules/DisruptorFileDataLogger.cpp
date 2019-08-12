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

/// \cond
#include <chrono>
#include <sstream>
/// \endcond

#include "Modules/DisruptorFileDataLogger.hpp"
#include "Modules/EventFormat.hpp"
#include "Utilities/Logging.hpp"


using namespace std::chrono_literals;
namespace daqutils = daqling::utilities;

extern "C" DisruptorFileDataLogger *create_object() { return new DisruptorFileDataLogger; }

extern "C" void destroy_object(DisruptorFileDataLogger *object) { delete object; }

DisruptorFileDataLogger::DisruptorFileDataLogger() : m_payloads{10000}, m_stopWriters{false}, m_bytes_sent{0}, m_bufferReady{-1}, m_fstream{0}
{
    INFO(__METHOD_NAME__);
    #warning RS -> Needs to be properly configured.
    // Set up static resources...
    m_writeBytes = 24 * daqutils::Constant::Kilo;  // 4K buffer writes
    std::ios_base::sync_with_stdio(false);
    m_fileNames[1] = "/tmp/test.bin"
    m_fileStreams[1][1].first = std::fstream("/tmp/event1.bin", std::ios::out | std::ios::binary);
    m_fileStreams[1][1].second = std::fstream("/tmp/event2.bin", std::ios::out | std::ios::binary);
    m_fileBuffers[1] = std::vector<daqling::utilities::Binary>(10);
    setup();
}

DisruptorFileDataLogger::~DisruptorFileDataLogger() 
{
    INFO(__METHOD_NAME__);
    // Tear down resources...
    m_stopWriters.store(true);
    //m_fileWriter.join();
    m_fileStreams[1].close();
}

void DisruptorFileDataLogger::start() 
{
    DAQProcess::start();
    INFO(" getState: " << getState());
    m_monitor_thread = std::make_unique<std::thread>(&DisruptorFileDataLogger::monitor_runner, this);
    m_fileWriter = std::make_unique<std::thread>(&DisruptorFileDataLogger::writeToFile, this, 1);
}

void DisruptorFileDataLogger::stop() 
{
    DAQProcess::stop();
    INFO(" getState: " << this->getState());
    m_monitor_thread->join();
    INFO("Joined successfully monitor thread");
}

void DisruptorFileDataLogger::runner() 
{
    INFO(" Running...");
    // auto& cm = daqling::core::ConnectionManager::instance();
  
    while (m_run)
    {
        daqutils::Binary pl(0);
        while (!m_connections.get(1, std::ref(pl)) && m_run)
        {
            std::this_thread::sleep_for(1ms);
        }
        m_payloads.write(pl);
        // DEBUG("Wrote data from channel 1...");
    }
    INFO(" Runner stopped");
}

#warning RS -> File rotation implementation is missing
#warning RS -> Hardcoded values should come from config.
void DisruptorFileDataLogger::setup() {
  // Loop through sources from config and add a file writer for each sink.
    int tid = 1;
    int l_bufferCounter = 0;
    m_fileWriters[tid] = std::make_unique<daqling::utilities::ReusableThread>(11111);
    m_bookKeeper = std::make_unique<daqling::utilities::ReusableThread>(11111);
    m_writeFunctors[tid] = [&, tid, l_bufferCounter] () mutable
    {
        int ftid = tid;
        INFO(" Spawning fileWriter for link: " << ftid);
        while (!m_stopWriters)
        {
            if (m_payloads.sizeGuess() > 0)
            {
                DEBUG(" SIZES: Queue pop.: " << m_payloads.sizeGuess()
                              << " loc.buff. size: " << m_fileBuffers[ftid][l_bufferCounter].size()
                              << " payload size: " << m_payloads.frontPtr()->size());
                long sizeSum = long(m_fileBuffers[ftid][l_bufferCounter].size()) + long(m_payloads.frontPtr()->size());
                INFO(sizeSum);
                if (sizeSum > m_writeBytes) //Split needed
                {
                    DEBUG(" Processing split.");
                    long splitSize = sizeSum - m_writeBytes;  // Calc split size
                    long splitOffset =
                        long(m_payloads.frontPtr()->size()) - long(splitSize);  // Calc split offset
                    DEBUG(" -> Sizes: | postPart: " << splitSize << " | For fillPart: " << splitOffset);
                    daqling::utilities::Binary fillPart(m_payloads.frontPtr()->startingAddress(),
                                              splitOffset);
                    DEBUG(" -> filPart DONE.");
                    daqling::utilities::Binary postPart(
                    static_cast<char *>(m_payloads.frontPtr()->startingAddress()) + splitOffset,
                    splitSize);
                    DEBUG(" -> postPart DONE.");
                    m_fileBuffers[ftid][l_bufferCounter] += fillPart;
                    m_bufferReady = l_bufferCounter;
                    DEBUG(" -> " << m_fileBuffers[ftid][l_bufferCounter].size() << " [Bytes] will be written.");
                    m_bytes_sent += m_fileBuffers[ftid][l_bufferCounter].size();
                    m_fileBuffers[ftid][l_bufferCounter] = postPart;  // Reset buffer to postPart.
                    m_payloads.popFront();           // Pop processed payload
                    INFO(l_bufferCounter);
                    l_bufferCounter = (l_bufferCounter+1) % m_fileBuffers[ftid].size(); //Cycling through the vector
                }
                else // We can safely extend the buffer.
                {
                    // This is fishy:
                    m_fileBuffers[ftid][l_bufferCounter] +=
                        *(reinterpret_cast<daqling::utilities::Binary *>(m_payloads.frontPtr()));
                    m_payloads.popFront();
                }
            }
            else //We haven't received any payloads yet
            {
                std::this_thread::sleep_for(1ms);
            }
        }
    };
    m_fileWriters[1]->set_work(m_writeFunctors[1]);
    m_bookKeeper->set_work(bookKeeper, ftid);
}

void DisruptorFileDataLogger::writeToFile(int ftid)
{ 
    int l_bufferReady;
    while(m_run)
    {
        if(m_bufferReady != -1)
        {
            l_bufferReady = m_bufferReady;

	        //extract binary payload and cast it to event header
            EventHeader* l_eh =  static_cast<EventHeader *>(m_fileBuffers[ftid][l_bufferReady].startingAddress();
	        //access the correct fstream according to event tag and write into it
            if(m_fstreamFull != 1)
            {
                m_fileStreams[ftid][l_eh->event_tag].first.write(static_cast<char *>(m_fileBuffers[ftid][l_bufferReady].startingAddress()),
                                    m_fileBuffers[ftid][l_bufferReady].size());  // write
                if(m_fileStreams[ftid][l_eh->event_tag].first.tellg() >= m_maxFileSize)
                    m_fstreamFull = 1; //change atomic value if the fstream is full, next time write to the other fstream if so

            }
            else
            {
                m_fileStreams[ftid][l_eh->event_tag].second.write(static_cast<char *>(m_fileBuffers[ftid][l_bufferReady].startingAddress()),
                                    m_fileBuffers[ftid][l_bufferReady].size());  // write
                if(m_fileStreams[ftid][l_eh->event_tag].second.tellg() >= m_maxFileSize)
                    m_fstreamFull = 2; //change atomic value if the fstream is full, next time write to the other fstream if so

            }
            m_bufferReady = -1;
        }
    } 
}

void bookKeeper(int ftid, EventHeader* l_eh)
{
    while(m_fstreamFull == 0) //none of the fstreams is full
    {
        std::this_thread::sleep_for(10ms);
    }
    switch(m_fstreamFull)
    {
        case 1: m_fileStreams[ftid][l_eh->event_tag].first.close()
                m_fileStreams[ftid][l_eh->event_tag].first.open("event_" + to_string((l_eh->event_tag)) + "file_"
                        + m_fileRotationCounters[l_eh->event_tag], std::ios::out | std::ios::binary);

                break;
        case 2: m_fileStreams[ftid][l_eh->event_tag].second.close()
                m_fileStreams[ftid][l_eh->event_tag].second.open("event_" + (to_string(l_eh->event_tag)) + "file_"
                        + m_fileRotationCounters[l_eh->event_tag], std::ios::out | std::ios::binary);
                break;
        default:
                break;
    }
    m_fileNames[l_eh->event_tag] = "event_" + (l_eh->event_tag.to_string()) + "file_"
                                   + m_fileRotationCounters[l_eh->event_tag];
    m_fileRotationCounters[l_eh->event_tag]++;
    m_fstreamFull = 0;
}

void DisruptorFileDataLogger::write() { INFO(" Should write..."); }

bool DisruptorFileDataLogger::write(uint64_t keyId, daqling::utilities::Binary &payload) {
    INFO(" Should write...");
    return false;
}


void DisruptorFileDataLogger::read() {}

void DisruptorFileDataLogger::shutdown() {}

void DisruptorFileDataLogger::monitor_runner() 
{
    while (m_run)
    {
        std::this_thread::sleep_for(1s);
        INFO("Write throughput: " << (double)m_bytes_sent / double(1000000) << " MBytes/s");
        INFO("Size guess " << m_payloads.sizeGuess());
        m_bytes_sent = 0;
    }
}


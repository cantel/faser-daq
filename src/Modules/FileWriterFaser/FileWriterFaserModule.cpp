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
#include <string>
/// \endcond

#include "FileWriterFaserModule.hpp"
#include "Utils/Logging.hpp"


using namespace std::chrono_literals;
namespace daqutils = daqling::utilities;

FileWriterFaserModule::FileWriterFaserModule() : m_payloads{10000}, m_bytes_sent{0}, m_bufferReady{-1}, m_fstreamFull{0}, m_stopWriters{false}
{
    INFO(__METHOD_NAME__);
    // Set up static resources...
    m_writeBytes = 24 * daqutils::Constant::Kilo;  // 24K buffer writes
    auto cfg = m_config.getConfig()["settings"];
    int l_configFileSize = cfg["maxFileSizeInGB"];
    m_maxFileSize = l_configFileSize * daqutils::Constant::Giga; //Get the file size we want to rotate at. IMPORTANT: Size from config is in GB!
    std::ios_base::sync_with_stdio(false);

    auto l_fileFormats = cfg["fileFormat"];
    if (l_fileFormats.empty())
    {
        ERROR("No file format specified");
        cfg.dump();
    }
    uint8_t l_event = 0;
    std::string l_fileNameFormat;
    for ( auto& fileFormat : l_fileFormats) //Opening initial file for each of the events
    {
        m_fileBuffers[l_event] = std::vector<daqling::utilities::Binary>(10);
        l_fileNameFormat = fileFormat["fileName"];
        m_fileNameFormats[l_event] = fileFormat["fileName"];
        m_fileStreams[1][l_event].first.open(l_fileNameFormat + std::to_string(m_fileRotationCounters[l_event]),
                                                std::ios::out | std::ios::binary);
        m_fileStreamsReady[l_event] = 1;
        m_fileNames[l_event] = l_fileNameFormat + std::to_string(m_fileRotationCounters[l_event]);
        m_fileRotationCounters[l_event]++;
        l_event++;
    }

    m_badEventsDumpFile.open(cfg["badEventsFileName"], std::ios::out | std::ios::binary);

    setup();
}

FileWriterFaserModule::~FileWriterFaserModule() 
{
    INFO(__METHOD_NAME__);
    // Tear down resources...
    m_stopWriters.store(true);
    m_fileWriter->join();
    m_bookKeeper->join();
    for(auto & outer_map_itr : m_fileStreams) //Closing all the fstreams
    {
        for(auto & inner_map_itr : outer_map_itr.second)
        {
            if(inner_map_itr.second.first)
                inner_map_itr.second.first.close();
            if(inner_map_itr.second.second)    
                inner_map_itr.second.second.close();
        }
    }
    m_badEventsDumpFile.close();
    INFO("EVERYTHING JOINED SUCCESSFULY");
}

void FileWriterFaserModule::start(int run_num) 
{
    DAQProcess::start(run_num);
    INFO(" getState: " << getState());
    m_monitor_thread = std::make_unique<std::thread>(&FileWriterFaserModule::monitor_runner, this);
    m_bookKeeper = std::make_unique<std::thread>(&FileWriterFaserModule::bookKeeper, this, 1); //1 is the channel
    m_fileWriter = std::make_unique<std::thread>(&FileWriterFaserModule::writeToFile, this, 1); //1 is the channel
}

void FileWriterFaserModule::stop() 
{
    DAQProcess::stop();
    INFO(" getState: " << this->getState());
    m_monitor_thread->join();
    INFO("Joined successfully monitor thread");
}

void FileWriterFaserModule::runner() 
{
    INFO(" Running...");
    while (m_run)
    {
        daqutils::Binary pl;
        while (!m_connections.get(1, std::ref(pl)) && m_run)
        {
            std::this_thread::sleep_for(1ms);
        }
	if (!m_run) break;
	try {
	  EventFull l_eh(pl);
	  m_eventTag = l_eh.event_tag(); //Extracting the current event tag (the event tag of the current payload)
	  size_t l_eventSize = l_eh.size();
       
	  if(m_fileRotationCounters.find(m_eventTag) == m_fileRotationCounters.end() || l_eventSize != pl.size()) 
	    throw std::runtime_error("Unknown or corrupted event");
	  m_payloads.write(pl);
	} catch (const std::runtime_error& e) {
	  ERROR("Got corrupted fragment: "<<e.what());
	      //handle
	      //handleBadEvent(l_eh);
	}
    }
    INFO(" Runner stopped");
}

void FileWriterFaserModule::setup() {
    // Loop through sources from config and add a file writer for each sink.
    int tid = 1;
    int l_bufferCounter = 0;
    m_fileWriters[tid] = std::make_unique<daqling::utilities::ReusableThread>(11111);

    m_writeFunctors[tid] = [&, tid, l_bufferCounter] () mutable
    {
        int ftid = tid;
        INFO(" Spawning fileWriter for link: " << ftid);
        while (!m_stopWriters)
        {
            if (m_payloads.sizeGuess() > 0)
            {
                DEBUG(" SIZES: Queue pop.: " << m_payloads.sizeGuess()
                              << " loc.buff. size: " << m_fileBuffers[m_eventTag][l_bufferCounter].size()
                              << " payload size: " << m_payloads.frontPtr()->size());
                long sizeSum = long(m_fileBuffers[m_eventTag][l_bufferCounter].size()) + long(m_payloads.frontPtr()->size()); //Calculating current event buffer size
                m_fileBuffers[m_eventTag][l_bufferCounter] += *(reinterpret_cast<daqling::utilities::Binary *>(m_payloads.frontPtr()));
                m_payloads.popFront(); // Pop processed payload
                if (sizeSum > m_writeBytes) //Writing is ready 
                {
                    m_bufferReady = l_bufferCounter;
                    DEBUG(" -> " << m_fileBuffers[m_eventTag][l_bufferCounter].size() << " [Bytes] will be written.");
                    m_bytes_sent += m_fileBuffers[m_eventTag][l_bufferCounter].size();
                    l_bufferCounter = (l_bufferCounter+1) % m_fileBuffers[m_eventTag].size(); //Cycling through the vector
                    m_fileBuffers[m_eventTag][l_bufferCounter] = daqutils::Binary(); //reset buffer to avoid multiple copies.
                }
            }
            else //We haven't received any payloads yet
            {
                std::this_thread::sleep_for(1ms);
            }
        }
    };
    m_fileWriters[1]->set_work(m_writeFunctors[1]);
}

void FileWriterFaserModule::writeToFile(int ftid)
{ 
    int l_bufferReady;
    uint8_t l_eventTag;
    while(m_run)
    {
        if(m_bufferReady != -1)
        {
            l_eventTag = m_eventTag;
            l_bufferReady = m_bufferReady;
            
            
            while(m_fileStreamsReady[m_eventTag] == 0 && m_run) //Waiting for the bookKeeper to open the files
            {
                INFO("sleeping");
                std::this_thread::sleep_for(1ms);
            }
	        //access the correct fstream according to event tag and write into it
            if(m_fstreamFull != 1)
            {
                m_fileStreams[ftid][l_eventTag].first.write(static_cast<char *>(m_fileBuffers[l_eventTag][l_bufferReady].data()),
                                    m_fileBuffers[l_eventTag][l_bufferReady].size());  // write
                if(m_fileStreams[ftid][l_eventTag].first.tellg() >= m_maxFileSize)
                    m_fstreamFull = 1; //change atomic value if the fstream is full, next time write to the other fstream if so

            }
            else
            {
                m_fileStreams[ftid][l_eventTag].second.write(static_cast<char *>(m_fileBuffers[l_eventTag][l_bufferReady].data()),
                                    m_fileBuffers[l_eventTag][l_bufferReady].size());  // write
                if(m_fileStreams[ftid][l_eventTag].second.tellg() >= m_maxFileSize)
                    m_fstreamFull = 2; //change atomic value if the fstream is full, next time write to the other fstream if so

            }
            m_bufferReady = -1;
        }
    } 
}

void FileWriterFaserModule::bookKeeper(int ftid)
{
    while(m_run)
    {
        while(m_fstreamFull == 0 && m_run) //none of the fstreams is full
        {
            std::this_thread::sleep_for(1ms);
        }
        switch(m_fstreamFull)
        {
            case 1: m_fileStreams[ftid][m_eventTag].first.close();
                    m_fileStreams[ftid][m_eventTag].first.open(m_fileNameFormats[m_eventTag] + std::to_string(m_fileRotationCounters[m_eventTag]),
                                                 std::ios::out | std::ios::binary);

                    break;
            case 2: m_fileStreams[ftid][m_eventTag].second.close();
                    m_fileStreams[ftid][m_eventTag].second.open(m_fileNameFormats[m_eventTag] + std::to_string(m_fileRotationCounters[m_eventTag]),
                                                 std::ios::out | std::ios::binary);
                    break;
            default:
                    break;
        }
        m_fileNames[m_eventTag] = m_fileNameFormats[m_eventTag] + std::to_string(m_fileRotationCounters[m_eventTag]);
        m_fileRotationCounters[m_eventTag]++;
        m_fstreamFull = 0;
    }
}
/**
void FileWriterFaserModule::handleBadEvent(const EventHeader* badEvent)
{
    m_badEventsDumpFile << *badEvent;
    m_badEventsDumpFile << "\n";
}*/

//ignore write, read and shutdown
void FileWriterFaserModule::write() { INFO(" Should write..."); }

bool FileWriterFaserModule::write(uint64_t /*keyId*/, daqling::utilities::Binary &/*payload*/) {
    INFO(" Should write...");
    return false;
}


void FileWriterFaserModule::read() {}

void FileWriterFaserModule::shutdown() {}

void FileWriterFaserModule::monitor_runner() 
{
    while (m_run)
    {
        std::this_thread::sleep_for(1s);
        INFO("Write throughput: " << (double)m_bytes_sent / double(1000000) << " MBytes/s");
        INFO("Size guess " << m_payloads.sizeGuess());
        m_bytes_sent = 0;
    }
}


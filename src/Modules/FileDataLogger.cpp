/// \cond
#include <chrono>
#include <sstream>
/// \endcond

#include "Modules/FileDataLogger.hpp"
#include "Utilities/Logging.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace std::chrono_literals;
namespace daqutils = daq::utilities;

extern "C" FileDataLogger *create_object() { return new FileDataLogger; }

extern "C" void destroy_object(FileDataLogger *object) { delete object; }

FileDataLogger::FileDataLogger() : m_payloads{10000}, m_randDevice{}, m_mt{m_randDevice()}, m_uniformDist{10240, 40960},
                                   m_stopWriters{false} { 
  INFO("FileDataLogger::FileDataLogger"); 
 
#warning RS -> Needs to be properly configured.
  // Set up static resources...
  std::ios_base::sync_with_stdio(false);
  m_fileNames[1] = "/tmp/test.bin";
  m_fileStreams[1] = std::fstream(m_fileNames[1], std::ios::out | std::ios::binary);
  m_fileBuffers[1] = daq::utilities::Binary(0);
  setup();
}

FileDataLogger::~FileDataLogger() { 
  INFO("FileDataLogger::~FileDataLogger"); 
  // Tear down resources...
}

void FileDataLogger::start() { 
  INFO(__METHOD_NAME__ << " getState: " << getState());
  m_runner_thread = std::make_unique<std::thread>(&FileDataLogger::runner, this);
}

void FileDataLogger::stop() { 
  INFO(__METHOD_NAME__ << " getState: " << this->getState());
  m_runner_thread->join();
  m_stopWriters.store(true);
}

void FileDataLogger::runner() {
  //auto& cm = daq::core::ConnectionManager::instance();
  uint64_t incr = 0;
  while (m_run) {
    std::this_thread::sleep_for(100ms);
    incr++;
    int randSize = m_uniformDist(m_mt);
    daqutils::Binary pl(randSize);
    m_payloads.write(pl);
  }
  DEBUG(__METHOD_NAME__ << " Runner stopped");
}

#warning RS -> File rotation implementation is missing
#warning RS -> Hardcoded values should come from config.
void FileDataLogger::setup() {
  // Loop through sources from config and add a file writer for each sink.
  int tid = 1;
  m_fileWriters[tid] = std::make_unique<daq::utilities::ReusableThread>(11111);
  m_writeFunctors[tid] = [&, tid]{
     int ftid = tid;
     INFO(__METHOD_NAME__ << " Spawning fileWriter for link: " << ftid);
     while( !m_stopWriters ){
       if ( m_payloads.sizeGuess() > 0 ) {
         DEBUG( " SIZES: " << m_fileBuffers[ftid].size() << " FRT PAYLOAD:" << m_payloads.frontPtr()->size() );         
         long sizeSum = long(m_fileBuffers[ftid].size()) + long(m_payloads.frontPtr()->size());
         if ( sizeSum > 1048576 ) {
           long splitSize = sizeSum-1048576;  //sizeDiff*long(-1);
           DEBUG("Need to write... AND SPLIT: " << splitSize << "Bytes will be post write part.");
           // NEED to split the binary
           long splitOffset = long(m_payloads.frontPtr()->size()) - long(splitSize);
           DEBUG(" SIZE FOR POSTPART: " << splitSize << " SIZE FOR FILL PART: " << splitOffset);          
           daq::utilities::Binary fillPart(m_payloads.frontPtr()->startingAddress(), splitOffset);
           DEBUG(" filPart DONE.");
           daq::utilities::Binary postPart(static_cast<char*>(m_payloads.frontPtr()->startingAddress()) + splitOffset, splitSize);
           DEBUG(" postPart DONE.");
           m_fileBuffers[ftid] += fillPart;
           // WRITE
           DEBUG("WRITING FROM " << (long long)m_fileBuffers[ftid].startingAddress() << " " << m_fileBuffers[ftid].size() << " Bytes will be written.");
           m_fileStreams[ftid].write((char*)m_fileBuffers[ftid].startingAddress(), m_fileBuffers[ftid].size());
           // RESET BUFFER TO POSTPART
           m_fileBuffers[ftid] = postPart;
           // POP 
           m_payloads.popFront();

         } else { // We can safely extend the buffer.

           m_fileBuffers[ftid] += *m_payloads.frontPtr();
           m_payloads.popFront();

         }
       }
       std::this_thread::sleep_for(100ms);
     }
  };
  m_fileWriters[1]->set_work(m_writeFunctors[1]);
}

void FileDataLogger::write() {
  INFO(__METHOD_NAME__ << " Should write...");
}

bool FileDataLogger::write(uint64_t keyId, daq::utilities::Binary& payload) {
  INFO(__METHOD_NAME__ << " Should write...");
  return false;
}

void FileDataLogger::read() {}

void FileDataLogger::shutdown() {}


// enrico.gamberini@cern.ch

#include "modules/BoardReader.hpp"
#include "utilities/Common.hpp"
#include "utilities/Logging.hpp"

#include <thread>
#include <chrono>

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace std::chrono_literals;

extern "C" BoardReader *create_object(std::string name, int num)
{
    return new BoardReader(name, num);
}

extern "C" void destroy_object(BoardReader *object)
{
    delete object;
}

BoardReader::BoardReader(std::string name, int num)
{
    INFO(__METHOD_NAME__ << " Passed " << name << " " << num << " with constructor");
    m_run = false;
    m_config.load("{ \"happy\": true, \"pi\": 3.141, \"foo\": \"bar\" }");
    INFO(__METHOD_NAME__ << " With config: " << m_config.dump() );
}

BoardReader::~BoardReader()
{
    INFO(__METHOD_NAME__ << " Clearing config: " << m_config.dump() );
    m_config.clear();
}

void BoardReader::start()
{
    INFO(__METHOD_NAME__ << " Changing config...");
    std::string key("bla");
    m_config.set(key, 42);
    

    INFO(__METHOD_NAME__ << " BINDING COMMAND SOCKET...");
    std::string connStr("tcp://localhost:5557");
    m_connections.setupCmdConnection(1, connStr);
     

    std::this_thread::sleep_for(60s);

    daq::utilities::Timer<std::chrono::milliseconds> msTimer;
    INFO(__METHOD_NAME__ << " Sleeping a bit with Timer...");
    msTimer.reset();
    std::this_thread::sleep_for(2s);
    INFO(__METHOD_NAME__ << " Elapsed time: " << msTimer.elapsed() << " ms");
}

void BoardReader::stop()
{
    INFO(__METHOD_NAME__ << " Fetching config...");
    std::string key("bla");
    int v = m_config.get<int>(key);
    INFO("  -> value: " << v);
}

void BoardReader::runner()
{
    while (m_run)
    {
        INFO(__METHOD_NAME__ << " Running...");
    }
}


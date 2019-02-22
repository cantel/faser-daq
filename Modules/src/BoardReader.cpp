// enrico.gamberini@cern.ch

#include "modules/BoardReader.hpp"
#include "utilities/Common.hpp"
#include "utilities/Logging.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

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


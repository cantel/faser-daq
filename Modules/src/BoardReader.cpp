// enrico.gamberini@cern.ch

#include "modules/BoardReader.hpp"
#include "utilities/Logging.hpp"

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
    INFO("Passed " << name << " " << num << " with constructor");
    m_run = false;
    m_config.load("{ \"happy\": true, \"pi\": 3.141, \"foo\": \"bar\" }");
    INFO("BoardReader::BoardReader() with config: " << m_config.dump() );
}

BoardReader::~BoardReader()
{
    INFO("BoardReader::~BoardReader() ... clearing config: " << m_config.dump() );
    m_config.clear();
}

void BoardReader::start()
{
    INFO("BoardReader::start, changing config...");
    std::string key("bla");
    m_config.set(key, 42);
}

void BoardReader::stop()
{
    INFO("BoardReader::stop, fetching config...");
    std::string key("bla");
    int v = m_config.get<int>(key);
    INFO("  -> value: " << v);
}

void BoardReader::runner()
{
    while (m_run)
    {
        INFO("BoardReader::runner");
    }
}

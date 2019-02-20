// enrico.gamberini@cern.ch

#include "modules/BoardReader.hpp"
#include "utilities/logging.hpp"

extern "C" BoardReader *create_object()
{
    return new BoardReader;
}

extern "C" void destroy_object(BoardReader *object)
{
    delete object;
}

BoardReader::BoardReader()
{
    m_run = false;
    m_config.load("blabla");
    INFO("BoardReader::BoardReader() with config: " << m_config.dump() );
}

BoardReader::~BoardReader()
{
    INFO("BoardReader::~BoardReader()");
}

void BoardReader::start()
{
    INFO("BoardReader::start");
}

void BoardReader::stop()
{
    INFO("BoardReader::stop");
}

void BoardReader::runner()
{
    while (m_run)
    {
        INFO("BoardReader::runner");
    }
}

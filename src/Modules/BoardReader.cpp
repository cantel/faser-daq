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
    INFO(__METHOD_NAME__ << " With config: " << m_config.dump() << " getState: " << this->getState() );
}

BoardReader::~BoardReader()
{
    INFO(__METHOD_NAME__);
}

void BoardReader::start()
{
    m_state = "running";
    INFO(__METHOD_NAME__ << " getState: " << getState() );

    m_run = true;
    m_runner_thread = std::make_unique<std::thread>(&BoardReader::runner, this);
    // std::this_thread::sleep_for(60s);

    // daq::utilities::Timer<std::chrono::milliseconds> msTimer;
    // INFO(__METHOD_NAME__ << " Sleeping a bit with Timer...");
    // msTimer.reset();
    // std::this_thread::sleep_for(2s);
    // INFO(__METHOD_NAME__ << " Elapsed time: " << msTimer.elapsed() << " ms");
}

void BoardReader::stop()
{
    m_run = false;
    m_state = "ready";
    INFO(__METHOD_NAME__ << " getState: " << this->getState() );
}

void BoardReader::runner()
{
    while (m_run)
    {
        INFO(__METHOD_NAME__ << " Running...");
        std::this_thread::sleep_for(2s);
    }
    INFO(__METHOD_NAME__ << " Runner stopped");
}


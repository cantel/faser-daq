// enrico.gamberini@cern.ch

#include "modules/EventBuilder.hpp"

#include <chrono>

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace std::chrono_literals;

extern "C" EventBuilder *create_object()
{
    return new EventBuilder;
}

extern "C" void destroy_object(EventBuilder *object)
{
    delete object;
}

EventBuilder::EventBuilder()
{
    m_run = false;
    INFO(__METHOD_NAME__ << " With config: " << m_config.dump() << " getState: " << this->getState() );
}

EventBuilder::~EventBuilder()
{
    INFO(__METHOD_NAME__);
}

void EventBuilder::start()
{
    INFO(__METHOD_NAME__ << " getState: " << getState() );

    m_run = true;
    m_runner_thread = std::make_unique<std::thread>(&EventBuilder::runner, this);
}

void EventBuilder::stop()
{
    m_run = false;
    INFO(__METHOD_NAME__ << " getState: " << this->getState() );
}

void EventBuilder::runner()
{
    auto &cm = daq::core::ConnectionManager::instance();
    while (m_run)
    {
        INFO(__METHOD_NAME__ << " Running...");
        std::this_thread::sleep_for(500ms);
        INFO("Received on channel 1 " << cm.getStr(1));
        INFO("Received on channel 2 " << cm.getStr(2));
    }
    INFO(__METHOD_NAME__ << " Runner stopped");
}

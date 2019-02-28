// enrico.gamberini@cern.ch

#include "modules/EventBuilder.hpp"
#include "utilities/Logging.hpp"

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
    INFO("EventBuilder::EventBuilder");
}

EventBuilder::~EventBuilder()
{
    INFO("EventBuilder::~EventBuilder");
}

void EventBuilder::start()
{
    INFO("EventBuilder::start");
}

void EventBuilder::stop()
{
    INFO("EventBuilder::stop");
}

void EventBuilder::runner()
{
    
}

#include "modules/CassandraDataLogger.hpp"
#include "utilities/Logging.hpp"

extern "C" CassandraDataLogger *create_object()
{
    return new CassandraDataLogger;
}

extern "C" void destroy_object(CassandraDataLogger *object)
{
    delete object;
}

CassandraDataLogger::CassandraDataLogger()
{
    INFO("CassandraDataLogger::CassandraDataLogger");
}

CassandraDataLogger::~CassandraDataLogger()
{
    INFO("CassandraDataLogger::~CassandraDataLogger");
}

void CassandraDataLogger::start()
{
    INFO("CassandraDataLogger::start");
}

void CassandraDataLogger::stop()
{
    INFO("CassandraDataLogger::stop");
}

void CassandraDataLogger::runner()
{
    
}

void CassandraDataLogger::setup() 
{

}

void CassandraDataLogger::write()
{

}

void CassandraDataLogger::read()
{

}

void CassandraDataLogger::shutdown()
{

}



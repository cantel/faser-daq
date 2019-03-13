#include "modules/FileDataLogger.hpp"
#include "utilities/Logging.hpp"

extern "C" FileDataLogger *create_object()
{
    return new FileDataLogger;
}

extern "C" void destroy_object(FileDataLogger *object)
{
    delete object;
}

FileDataLogger::FileDataLogger()
{
    INFO("FileDataLogger::FileDataLogger");
}

FileDataLogger::~FileDataLogger()
{
    INFO("FileDataLogger::~FileDataLogger");
}

void FileDataLogger::start()
{
    INFO("FileDataLogger::start");
}

void FileDataLogger::stop()
{
    INFO("FileDataLogger::stop");
}

void FileDataLogger::runner()
{
    
}

void FileDataLogger::setup() 
{

}

void FileDataLogger::write()
{

}

bool FileDataLogger::write(uint64_t keyId, daq::utilities::Binary& payload)
{

}

void FileDataLogger::read()
{

}

void FileDataLogger::shutdown()
{

}



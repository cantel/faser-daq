#include <fstream>
#include "Modules/DisruptorFileDataLogger.hpp"
#include "Utilities/Logging.hpp"


extern "C" DisruptorFileDataLogger *create_object() { return new DisruptorFileDataLogger; }

extern "C" void destroy_object(DisruptorFileDataLogger *object) { delete object; }


DisruptorFileDataLogger::DisruptorFileDataLogger()
{


}

DisruptorFileDataLogger::~DisruptorFileDataLogger() {  }

void DisruptorFileDataLogger::start()
{
    INFO("DisruptorFileDataLogger::start");
    daqling::core::DAQProcess::start();
}

void DisruptorFileDataLogger::stop()
{
    daqling::core::DAQProcess::stop();
    INFO("DisruptorFileDataLogger::stop");
}

void DisruptorFileDataLogger::runner()
{
    INFO("DisruptorFileDataLogger::Running...");
    FILE* lOutPutFile = fopen("result", "wb");
    
    daqling::utilities::Binary pl(0);
    

    while (m_run)
    {
        INFO("DisruptorFileDataLogger::m_run");
        while (!m_connections.get(1, std::ref(pl)) && m_run)
        {
	    std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

	INFO("DisruptorFileDataLogger::m_connections");
	WriteToBuffer(pl);
        WriteToFile(lOutPutFile);
    }
    fclose(lOutPutFile);
    INFO(" Runner stopped");
}

void DisruptorFileDataLogger::WriteToFile(FILE* outputFile)
{
    INFO("DisruptorFileDataLogger::Writing to file");
    INFO(mEventBuffer.GetNumOfFreeToReadSlots());
    if(mEventBuffer.GetNumOfFreeToReadSlots() >= mReadSlots) //The Chunk available to be written to a file is large enough
    {
	INFO("DisruptorFileDataLogger::Writing to file2");
        mEventBuffer.Read(outputFile, mReadSlots);
    }
}

void DisruptorFileDataLogger::WriteToBuffer(daqling::utilities::Binary pl)
{
    mEventBuffer.Write(pl);
}

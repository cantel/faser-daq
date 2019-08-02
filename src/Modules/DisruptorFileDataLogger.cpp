#include <fstream>
#include "Modules/DisruptorFileDataLogger.hpp"
#include "Utilities/Logging.hpp"
#include <string>

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
    
    daqling::utilities::Binary pl(0);

    //FILE* lOutPutFile = fopen("result", "w");
    
    std::ofstream lOutPutFile("result", std::ofstream::binary);

    std::thread TWriteToFile(&DisruptorFileDataLogger::WriteToFile, this, std::ref(lOutPutFile));
    std::thread TWriteToBuffer(&DisruptorFileDataLogger::WriteToBuffer, this, pl);
    
    TWriteToFile.join();
    TWriteToBuffer.join();
 
    lOutPutFile.close();
    
    INFO(" Runner stopped");
}

void DisruptorFileDataLogger::WriteToFile(std::ofstream& outputFile)
{
    while (m_run)
    {
        if(mEventBuffer.GetNumOfFreeToReadSlots() > mReadSlots) //The Chunk available to be written to a file is large enough
        {
            mEventBuffer.Read(outputFile, mReadSlots);
        }
    }
}

void DisruptorFileDataLogger::WriteToBuffer(daqling::utilities::Binary pl)
{
    while (m_run)
    {
        INFO("DisruptorFileDataLogger::m_run");
        while (!m_connections.get(1, std::ref(pl)) && m_run)
        {
	    std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
	INFO("DisruptorFileDataLogger::m_connections");
	mEventBuffer.Write(pl);
    }
}




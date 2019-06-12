#include <fstream>
#include "DisruptorFileDataLogger.hpp"



extern "C" DisruptorFileDataLogger *create_object() { return new DisruptorFileDataLogger; }

extern "C" void destroy_object(DisruptorFileDataLogger *object) { delete object; }


DisruptorFileDataLogger::DisruptorFileDataLogger()
{


}

DisruptorFileDataLogger::~DisruptorFileDataLogger() {  }

void DisruptorFileDataLogger::start()
{
    daqling::core::DAQProcess::start();
}

void DisruptorFileDataLogger::stop()
{
    daqling::core::DAQProcess::stop();
}

void DisruptorFileDataLogger::runner()
{

    std::thread lWriterToBuffer(&DisruptorFileDataLogger::WriteToBuffer);
    std::thread lWriterToFile(&DisruptorFileDataLogger::WriteToFile);
    std::ofstream lOutPutFile("result.txt");
    while (m_run) {
        daqling::utilities::Binary pl(0);
        while (!m_connections.get(1, std::ref(pl))) {
            std::this_thread::sleep_for(10ms);
        }
    }
    lWriterToBuffer.join();
    lWriterToFile.join();
}

void DisruptorFileDataLogger::WriteToFile(std::ofstream outputFile)
{
    if(mEventBuffer.GetNumOfFreeToReadSlots() >= mReadSlots) //The Chunk available to be written to a file is large enough
    {
        mEventBuffer.Read(outputFile, mReadSlots);
    }
}

void DisruptorFileDataLogger::WriteToBuffer(daqling::utilities::Binary pl)
{
    mEventBuffer.Write(pl);
}

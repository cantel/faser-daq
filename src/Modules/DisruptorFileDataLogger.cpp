#include <fstream>
#include "Modules/DisruptorFileDataLogger.hpp"



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
    FILE* lOutPutFile = fopen("result.txt", "wb");
    std::thread lWriterToFile = std::thread(&DisruptorFileDataLogger::WriteToFile, this, lOutPutFile);
    daqling::utilities::Binary pl(0);
    std::thread lWriterToBuffer = std::thread(&DisruptorFileDataLogger::WriteToBuffer, this, pl);
    while (m_run) {
        while (!m_connections.get(1, std::ref(pl))) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    lWriterToBuffer.join();
    lWriterToFile.join();
}

void DisruptorFileDataLogger::WriteToFile(FILE* outputFile)
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

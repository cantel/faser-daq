#ifndef DISRUPTORFILEDATALOGGER_DISRUPTORFILEDATALOGGER_HPP
#define DISRUPTORFILEDATALOGGER_DISRUPTORFILEDATALOGGER_HPP

#include "daq/daqling/include/Core/DAQProcess.hpp"
#include "daq/daqling/include/Utilities/Binary.hpp"
#include "RingBuffer.hpp"
#include <thread>
#define BUFFER_SIZE 256

class DisruptorFileDataLogger : public daqling::core::DAQProcess
{
public:
    DisruptorFileDataLogger();
    ~DisruptorFileDataLogger();
    void start();
    void stop();
    void runner();


private:
    const int mReadSlots = 200;
    RingBuffer<daqling::utilities::Binary, BUFFER_SIZE> mEventBuffer;
    void WriteToFile(std::ofstream outputFile);
    void WriteToBuffer(daqling::utilities::Binary pl);
};

#endif //DISRUPTORFILEDATALOGGER_DISRUPTORFILEDATALOGGER_HPP

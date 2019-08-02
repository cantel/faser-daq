#ifndef FILEDATALOGGER_RINGBUFFER_HPP
#define FILEDATALOGGER_RINGBUFFER_HPP

#include <iostream>
#include <fstream>
#include "Utilities/Logging.hpp"
#include <chrono>

using namespace std::chrono;

template <class T, size_t size>
class RingBuffer
{
public:
    RingBuffer() : mReadIndex(-1), mWriteIndex(0)
    {}

    ~RingBuffer()
    {
/**
        for (int i = 0; i < mSize; ++i)
        {
            delete mRingBuffer[i];
        }*/
        delete[] mRingBuffer;
    }

    bool Write(const T& data)
    {
        unsigned int lWriteIndex = mWriteIndex;
        if(lWriteIndex == mReadIndex)
            return false;
        //INFO("I'M ABOUT TO WRITEEEE");
        mRingBuffer[lWriteIndex] = data;
        lWriteIndex = IncrementIndex(lWriteIndex, 1);

        mWriteIndex = lWriteIndex;

        return true;
    }

    bool Read(std::ofstream& out, size_t slotsToRead)
    {
	if(!out)
	    return false;

        unsigned int lReadIndex = (mReadIndex+1) % mSize;
        if(lReadIndex == mWriteIndex) //problematic
            return false; 
        if(GetNumOfFreeToReadSlots() < slotsToRead) //problematic
            return false;
        //We have enough slots to read
        if(mSize - lReadIndex < slotsToRead) //Our read needs to circle //problematic
        {
            INFO("I'M ABOUT TO WRITE");
            size_t lBytesToRead = (mSize - lReadIndex) * sizeof(T);
            size_t lRemainderToRead = ( slotsToRead - (mSize - lReadIndex) )* sizeof(T);
   
            //fwrite(mRingBuffer + lReadIndex, sizeof(T), lBytesToRead, out);
            //fwrite(mRingBuffer + lReadIndex + lBytesToRead, sizeof(T), lRemainderToRead, out);

        }
        else
        {
            //INFO("I'M ABOUT TO WRITE 2");
            out.write(static_cast<char*>(mRingBuffer[lReadIndex].startingAddress()), mRingBuffer[lReadIndex].size());
        }

        lReadIndex = IncrementIndex(lReadIndex, slotsToRead-1);

        mReadIndex = lReadIndex;
        return true;
    }

    unsigned int GetNumOfFreeToReadSlots()
    {
	return mWriteIndex+1 < mReadIndex+1 ? mSize - (mReadIndex+1 - mWriteIndex) : (mWriteIndex-1 - mReadIndex);
    }


private:
    unsigned int mReadIndex;
    unsigned int mWriteIndex;
    const unsigned int mSize = size;

    T mRingBuffer[size];

    unsigned int IncrementIndex(unsigned int currIndex, unsigned int amountToIncrease)
    {
        return (currIndex + amountToIncrease) % mSize;
    }
};

#endif //FILEDATALOGGER_RINGBUFFER_HPP

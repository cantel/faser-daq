#ifndef FILEDATALOGGER_RINGBUFFER_HPP
#define FILEDATALOGGER_RINGBUFFER_HPP

#include <iostream>
#include <stdio.h>
#include "Utilities/Logging.hpp"

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
        volatile unsigned int lWriteIndex = mWriteIndex;
        if(lWriteIndex == mReadIndex && mWriteFlag == true)
            return false;

        mRingBuffer[lWriteIndex] = data;
        lWriteIndex = IncrementIndex(lWriteIndex, 1);

        mWriteIndex = lWriteIndex;
	mWriteFlag = true;

        return true;
    }

    bool Read(FILE* out, size_t slotsToRead)
    {
	if(!out)
	    return false;

        volatile unsigned int lReadIndex = mReadIndex+1;
        if(lReadIndex == mWriteIndex)
            return false;
        if(GetNumOfFreeToReadSlots() < slotsToRead)
            return false;
        //We have enough slots to read
        if(mSize - lReadIndex < slotsToRead) //Our read needs to circle
        {
            INFO("I'M ABOUT TO WRITE");
            size_t lBytesToRead = (mSize - lReadIndex) * sizeof(T);
            size_t lRemainderToRead = ( slotsToRead - (mSize - lReadIndex) )* sizeof(T);
            fwrite(mRingBuffer + lReadIndex, sizeof(T), lBytesToRead, out);
            fwrite(mRingBuffer + lReadIndex + lBytesToRead, sizeof(T), lRemainderToRead, out);
        }
        else
        {
            INFO("I'M ABOUT TO WRITE 2");
            INFO(sizeof(T));
            INFO(lReadIndex);
            INFO(slotsToRead);
            fwrite(mRingBuffer + lReadIndex, sizeof(T), slotsToRead, out);
        }

        lReadIndex = IncrementIndex(lReadIndex, slotsToRead-1);

        mReadIndex = lReadIndex;
        return true;
    }

    unsigned int GetNumOfFreeToReadSlots()
    {
        return abs(int((mWriteIndex+1) - (mReadIndex+1))); //one of the indexes might be zero, so we add one to both.
    }


private:
    unsigned int mReadIndex;
    unsigned int mWriteIndex;
    const unsigned int mSize = size;
    bool mWriteFlag = false;
    T mRingBuffer[size];

    unsigned int IncrementIndex(unsigned int currIndex, unsigned int amountToIncrease)
    {
        return (currIndex + amountToIncrease) % mSize;
    }
};

#endif //FILEDATALOGGER_RINGBUFFER_HPP

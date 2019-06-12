#ifndef FILEDATALOGGER_RINGBUFFER_HPP
#define FILEDATALOGGER_RINGBUFFER_HPP

#include <iostream>

template <class T, size_t size>
class RingBuffer
{
public:
    RingBuffer() : mReadIndex(0), mWriteIndex(0)
    {}

    ~RingBuffer()
    {
        for (int i = 0; i < mSize; ++i)
        {
            delete mRingBuffer[i];
        }
        delete[] mRingBuffer;
    }

    bool Write(const T& data)
    {
        volatile unsigned int lWriteIndex = mWriteIndex;
        if(lWriteIndex == mReadIndex)
            return false;

        mRingBuffer[lWriteIndex] = data;
        lWriteIndex = IncrementIndex(lWriteIndex, 1);

        mWriteIndex = lWriteIndex;

        return true;
    }

    bool Read(std::ofstream& out, size_t slotsToRead)
    {
        volatile unsigned int lReadIndex = mReadIndex;
        if(lReadIndex == mWriteIndex)
            return false;
        if(GetNumOfFreeToReadSlots() < slotsToRead)
            return false;
        //We have enough slots to read
        if(mSize - lReadIndex < slotsToRead) //Our read needs to circle
        {
            size_t lBytesToRead = (mSize - lReadIndex) * sizeof(T);
            size_t lRemainderToRead = ( slotsToRead - (mSize - lReadIndex) )* sizeof(T);
            fwrite(mRingBuffer + lReadIndex, sizeof(T), lBytesToRead, out);
            fwrite(mRingBuffer + lReadIndex + lBytesToRead, sizeof(T), lRemainderToRead, out);
        }
        else
        {
            fwrite(mRingBuffer + lReadIndex, sizeof(T), slotsToRead, out);
        }

        lReadIndex = IncrementIndex(lReadIndex, slotsToRead);

        mReadIndex = lReadIndex;
        return true;
    }

    unsigned int GetNumOfFreeToReadSlots()
    {
        return abs((mWriteIndex+1) - (mReadIndex+1)); //one of the indexes might be zero, so we add one to both.
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

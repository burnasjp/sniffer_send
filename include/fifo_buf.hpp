#pragma once

#include <cstdint>
#include <cstddef>

template <typename type>
void memcpy0(type *pSrc, type *pDst, size_t size)
{
    while (size)
    {
        *pDst++ = *pSrc++;
        size--;
    }
}

// TODO DMA転送に適した構造を検討する
template <int capacity, typename type = uint8_t>
class fifo_buf
{
private:
    type buf[capacity];
    volatile int buf_pos_out = 0;
    volatile int buf_pos_in = 0;

private:
    //入力ポジション。
    void in_step(int step = 1)
    {
        buf_pos_in += step;
        if (buf_pos_in >= capacity)
        {
            buf_pos_in -= capacity;
        }
    }

    //出力ポジション。
    void out_step(int step = 1)
    {
        buf_pos_out += step;
        if (buf_pos_out >= capacity)
        {
            buf_pos_out -= capacity;
        }
    }

public:
    bool empty()
    {
        return buf_pos_in == buf_pos_out;
    }

    size_t getCapacity()
    {
        return capacity - 1;
    }
    //バッファリングされているデータのサイズを返す。
    size_t getLength()
    {
        int l_bus_pos_in = buf_pos_in;
        if (buf_pos_out <= l_bus_pos_in)
            return buf_pos_in - buf_pos_out;
        else
            return (capacity - buf_pos_out) + l_bus_pos_in;
    }

    //入力側のポインタを返す。
    type *getInPosPointer()
    {
        return &buf[buf_pos_in];
    }

    //出力側のポインタを返す。
    type *getOutPosPointer()
    {
        return &buf[buf_pos_out];
    }

    // FIFOの空き領域のサイズを返す。
    size_t getFreeSpace()
    {
        return getCapacity() - getLength();
    }

    //一度に取得できる単一最大のブロックサイズを返却する。
    size_t getNextOutMaxBlockSize()
    {
        return buf_pos_out < buf_pos_in ? (buf_pos_in - buf_pos_out) : (capacity - buf_pos_out);
    }

    //一度に書き込みできる単一最大のブロックサイズを返却する。
    size_t getNextInMaxBlockSize()
    {
        return buf_pos_in < buf_pos_out ? (buf_pos_out - buf_pos_in) : (capacity - buf_pos_in);
    }

    //バッファにデータを格納する。
    bool push_back(type data)
    {
        //バッファに収まらない場合は何もせずreturn
        if (getFreeSpace() == 0)
        {
            return false;
        }
        buf[buf_pos_in] = data;
        in_step();
        return true;
    }

    bool push_back(type *data, size_t size)
    {
        //バッファに収まらない場合はfalseを返す。
        if (size > getFreeSpace())
        {
            return false;
        }
        while (size)
        {
            size_t block_size = getNextInMaxBlockSize();
            if (size < block_size)
            {
                block_size = size;
            }

            memcpy0(data, &buf[buf_pos_in],
                    block_size * sizeof(type));
            in_step(block_size);

            data += block_size;
            size -= block_size;
        }
        return true;
    }

    type &front()
    {
        return *getOutPosPointer();
    }

    size_t pop_front()
    {
        if (empty())
            return 0;
        out_step();
        return 1;
    }

    size_t pop_front(size_t size, type *pDst)
    {
        size_t length = max(size, getNextOutMaxBlockSize());

        memcpy0(getOutPosPointer(), pDst, length);
        out_step(length);

        if (length <= size)
            return length;

        pDst += length;

        size_t remain = size - length;
        size_t length2 = max(remain, getNextOutMaxBlockSize());
        memcpy0(getOutPosPointer(), pDst, length2);

        return length + length2;
    }
};

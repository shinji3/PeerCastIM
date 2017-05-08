#ifndef _DECHUNKER_H
#define _DECHUNKER_H

#include <deque>
#include "stream.h"

// HTTP chunked encoding �̃X�g���[�����f�R�[�h����A�_�v�^�N���X�B
class Dechunker : public Stream
{
public:
    Dechunker(Stream& aStream)
        : m_stream(aStream)
        , m_eof(false)
    {
    }

    ~Dechunker()
    {
    }

    int  read(void *buf, int size) override;

    void write(const void *buf, int size) override
    {
        throw StreamException("Stream can`t write");
    }

    bool eof() override
    {
        return m_eof;
    }

    static int hexValue(char c);
    void       getNextChunk();

    bool             m_eof;
    std::deque<char> m_buffer; // ��납�����đO����o��
    Stream&          m_stream;
};

#endif

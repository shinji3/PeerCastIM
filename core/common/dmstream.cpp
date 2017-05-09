#include "dmstream.h"
#include <algorithm>
#include <string>

#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif

DynamicMemoryStream::DynamicMemoryStream()
    : m_pos(0)
{
}

void  DynamicMemoryStream::checkSize(int size)
{
    if (size > (int)m_buffer.size())
        while ((int)m_buffer.size() < size)
            m_buffer.push_back(0);
}

int  DynamicMemoryStream::read(void *buf, int count)
{
    if (m_pos == m_buffer.size())
        throw StreamException("End of stream");

    auto end = std::copy(m_buffer.begin() + m_pos,
                         m_buffer.begin() + (std::min)(m_pos + count, (int) m_buffer.size()),
                         static_cast<char*>(buf));
    auto bytesRead = (int)(end - static_cast<char*>(buf));
    m_pos += bytesRead;
    return bytesRead;
}

void DynamicMemoryStream::write(const void *buf, int count)
{
    checkSize(m_pos + count);
    std::copy(static_cast<const char*>(buf),
              static_cast<const char*>(buf) + count,
              m_buffer.begin() + m_pos);
    m_pos += count;
}

bool DynamicMemoryStream::eof()
{
    return m_pos == m_buffer.size();
}

void DynamicMemoryStream::rewind()
{
    m_pos = 0;
}

void DynamicMemoryStream::seekTo(int newPos)
{
    checkSize(newPos);
    m_pos = newPos;
}

int  DynamicMemoryStream::getPosition()
{
    return m_pos;
}

int  DynamicMemoryStream::getLength()
{
    return (int)m_buffer.size();
}

std::string DynamicMemoryStream::str()
{
    return std::string(m_buffer.begin(), m_buffer.end());
}

void DynamicMemoryStream::str(const std::string& data)
{
    m_buffer = data;
    m_pos = 0;
}

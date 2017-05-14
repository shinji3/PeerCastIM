#include <algorithm>
#include <iterator>

#include "dechunker.h"
#include "defer.h"

#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif

using namespace std;

int Dechunker::hexValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 0xa;
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 0xA;
    else
        return -1;
}

// �ǂݍ��߂��������Ԃ��Ă������̂��Ȃ��c�Hread �̎d�l���킩��Ȃ��B
// MemoryStream ���Ɨv�����ꂽ�����̃f�[�^���Ȃ������� 0 ��Ԃ���
// ���邪�AFileStream ���Ɠǂ߂��������ǂ�ł���B
//
// �������� size �o�C�g�ǂ߂�܂Ńu���b�N���ė~�����BUClientSocket
// �͂����������ɂȂ��Ă�ȁB�㗬��ClientSocket���{�Ԋ�������A
// ����ł������B
int  Dechunker::read(void *buf, int size)
{
    if (m_eof)
        throw StreamException("Closed on read");

    char *p = (char*) buf;

    while (true)
    {
        if ((int)m_buffer.size() >= size)
        {
            while (size > 0)
            {
                *p++ = m_buffer.front();
                m_buffer.pop_front();
                size--;
            }
            int r = (int)(p - (char*)buf);
            updateTotals(r, 0);
            return r;
        } else {
            getNextChunk();
            continue;
        }
    }
}

void Dechunker::getNextChunk()
{
    size_t size = 0;

    // �`�����N�T�C�Y��ǂݍ��ށB
    while (true)
    {
        char c = m_stream.readChar();
        if (c == '\r')
        {
            c = m_stream.readChar();
            if (c != '\n')
                throw StreamException("Protocol error");
            break;
        }
        int v = hexValue(c);
        if (v < 0)
            throw StreamException("Protocol error");
        size *= 0x10;
        size += v;
    }

    if (size == 0)
    {
        if (m_stream.readChar() != '\r' || m_stream.readChar() != '\n')
            throw StreamException("Protocol error");
        m_eof = true;
        throw StreamException("Closed on read");
    }

    char *buf = new char[size];
    Defer cleanup([=]() { delete[] buf; });
    int r;

    r = m_stream.read(buf, (int)size);
    if (r != size)
    {
        throw StreamException("Premature end");
    }
    copy(buf, buf + size, back_inserter(m_buffer));

    if (m_stream.readChar() != '\r' || m_stream.readChar() != '\n')
        throw StreamException("Protocol error");
}

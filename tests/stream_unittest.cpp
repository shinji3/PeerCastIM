#include "stdafx.h"
#include "CppUnitTest.h"

#include "stream.h"
#include "sstream.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

class MockStream : public Stream
{
public:
    MockStream()
    {
        readCount = 0;
        writeCount = 0;
    }

    int read(void *p, int len) override
    {
        readCount++;
        for (int i = 0; i < len; i++)
            ((char*)p)[i] = (i & 1) ? 'B' : 'A';
        return len;
    }

    void write(const void* p, int len) override
    {
        writeCount++;
        lastWriteData = std::string((char*)p, (char*)p + len);
    }

    int readCount;
    int writeCount;
    std::string lastWriteData;
};

namespace StreamFixture
{
    TEST_CLASS(StreamFixture)
    {
    public:

        static void forward_write_vargs(Stream& stream, const char* fmt, ...)
        {
            va_list ap;
            va_start(ap, fmt);
            stream.write(fmt, ap);
            va_end(ap);
        }

        MockStream s;
        StringStream mem;

        TEST_METHOD(StreamFixture_readUpto)
        {
            Assert::AreEqual(0, s.readUpto(NULL, 0));
        }

        TEST_METHOD(StreamFixture_eof)
        {
            ASSERT_THROW(s.eof(), StreamException);
        }

        TEST_METHOD(StreamFixture_rewind)
        {
            ASSERT_THROW(s.rewind(), StreamException);
        }

        TEST_METHOD(StreamFixture_seekTo)
        {
            ASSERT_THROW(s.seekTo(0), StreamException);
        }

        TEST_METHOD(StreamFixture_writeTo)
        {
            StringStream mem;
            Assert::AreEqual(0, s.readCount);
            s.writeTo(mem, 1);
            Assert::AreEqual(1, s.readCount);
            Assert::AreEqual("A", mem.str().c_str());
        }

        TEST_METHOD(StreamFixture_skip)
        {
            Assert::AreEqual(0, s.readCount);

            ASSERT_NO_THROW(s.skip(0));
            Assert::AreEqual(0, s.readCount);

            ASSERT_NO_THROW(s.skip(1));
            Assert::AreEqual(1, s.readCount);
        }

        TEST_METHOD(StreamFixture_close)
        {
            ASSERT_NO_THROW(s.close());
        }

        TEST_METHOD(StreamFixture_setReadTimeout)
        {
            ASSERT_NO_THROW(s.setReadTimeout(0));
        }

        TEST_METHOD(StreamFixture_setWriteTimeout)
        {
            ASSERT_NO_THROW(s.setWriteTimeout(0));
        }

        TEST_METHOD(StreamFixture_setPollRead)
        {
            ASSERT_NO_THROW(s.setPollRead(false));
            ASSERT_NO_THROW(s.setPollRead(true));
        }

        TEST_METHOD(StreamFixture_getPosition)
        {
            Assert::AreEqual(0, s.getPosition());
        }

        TEST_METHOD(StreamFixture_readChar)
        {
            Assert::AreEqual('A', s.readChar());
        }

        TEST_METHOD(StreamFixture_readShort)
        {
            Assert::AreEqual(0x4241, s.readShort());
        }

        TEST_METHOD(StreamFixture_readLong)
        {
            Assert::AreEqual(0x42414241, s.readLong());
        }

        TEST_METHOD(StreamFixture_readInt)
        {
            Assert::AreEqual(0x42414241, s.readInt());
        }

        TEST_METHOD(StreamFixture_readID4)
        {
            Assert::AreEqual("ABAB", s.readID4().getString().str());
        }

        TEST_METHOD(StreamFixture_readInt24)
        {
            Assert::AreEqual(0x00414241, s.readInt24());
        }

        TEST_METHOD(StreamFixture_readTag)
        {
            Assert::AreEqual('ABAB', s.readTag());
        }

        TEST_METHOD(StreamFixture_readString)
        {
            char buf[6] = "";
            Assert::AreEqual(5, s.readString(buf, 5));
            Assert::AreEqual(5, s.readCount);
            Assert::AreEqual("AAAAA", buf);
        }

        TEST_METHOD(StreamFixture_readReady)
        {
            Assert::IsTrue(s.readReady(0));
        }

        TEST_METHOD(StreamFixture_numPending)
        {
            Assert::AreEqual(0, s.numPending());
        }

        TEST_METHOD(StreamFixture_writeID4)
        {
            ID4 id("ruby");
            s.writeID4(id);
            Assert::AreEqual("ruby", s.lastWriteData.c_str());
        }

        TEST_METHOD(StreamFixture_writeChar)
        {
            s.writeChar('Z');
            Assert::AreEqual("Z", s.lastWriteData.c_str());
        }

        TEST_METHOD(StreamFixture_writeShort)
        {
            s.writeShort(0x4142);
            Assert::AreEqual("BA", s.lastWriteData.c_str());
        }

        TEST_METHOD(StreamFixture_writeLong)
        {
            s.writeLong(0x41424344);
            Assert::AreEqual("DCBA", s.lastWriteData.c_str());
        }

        TEST_METHOD(StreamFixture_writeInt)
        {
            s.writeInt(0x41424344);
            Assert::AreEqual("DCBA", s.lastWriteData.c_str());
        }

        TEST_METHOD(StreamFixture_writeTag)
        {
            s.writeTag("ABCD");
            Assert::AreEqual("ABCD", s.lastWriteData.c_str());
        }

        TEST_METHOD(StreamFixture_writeUTF8_null)
        {
            mem.writeUTF8(0x00);
            Assert::AreEqual(1, mem.getLength());
            Assert::AreEqual("", mem.str().c_str());
        }

        TEST_METHOD(StreamFixture_writeUTF8_1)
        {
            mem.writeUTF8(0x41);
            Assert::AreEqual(1, mem.getLength());
            Assert::AreEqual("A", mem.str().c_str());
        }

        TEST_METHOD(StreamFixture_writeUTF8_2)
        {
            mem.writeUTF8(0x3b1);
            Assert::AreEqual(2, mem.getLength());
            Assert::AreEqual("α", mem.str().c_str());
        }

        TEST_METHOD(StreamFixture_writeUTF8_3)
        {
            mem.writeUTF8(0x3042);
            Assert::AreEqual(3, mem.getLength());
            Assert::AreEqual("あ", mem.str().c_str());
        }

        TEST_METHOD(StreamFixture_writeUTF8_4)
        {
            mem.writeUTF8(0x1f4a9);
            Assert::AreEqual(4, mem.getLength());
            Assert::AreEqual("💩", mem.str().c_str());
        }

        TEST_METHOD(StreamFixture_readLine)
        {
            char buf[1024];

            memset(buf, 0, 1024);
            mem.str("abc");
            ASSERT_THROW(mem.readLine(buf, 1024), StreamException);

            memset(buf, 0, 1024);
            mem.str("abc\ndef");
            Assert::AreEqual(3, mem.readLine(buf, 1024));
            Assert::AreEqual("abc", buf);

            memset(buf, 0, 1024);
            mem.str("abc\r\ndef");
            Assert::AreEqual(3, mem.readLine(buf, 1024));
            Assert::AreEqual("abc", buf);

            // CR では停止しない
            memset(buf, 0, 1024);
            mem.str("abc\rdef");
            ASSERT_THROW(mem.readLine(buf, 1024), StreamException);

            // 行中の CR は削除される
            memset(buf, 0, 1024);
            mem.str("abc\rdef\r\n");
            Assert::AreEqual(6, mem.readLine(buf, 1024));
            Assert::AreEqual("abcdef", buf);
        }

        TEST_METHOD(StreamFixture_readLine_max0)
        {
            char buf[2] = "A";

            mem.str("ab\r\n");
            Assert::AreEqual(0, mem.readLine(buf, 0));
            Assert::AreEqual("A", buf);
        }

        TEST_METHOD(StreamFixture_readLine_max1)
        {
            char buf[2] = { 'A', 'A' };

            mem.str("ab\r\n");
            Assert::AreEqual(0, mem.readLine(buf, 1));
            Assert::AreEqual("", buf);
        }

        TEST_METHOD(StreamFixture_readLine_max2)
        {
            char buf[3];

            memset(buf, 'A', sizeof(buf));
            mem.str("ab\r\n");
            Assert::AreEqual(1, mem.readLine(buf, 2));
            Assert::AreEqual("a", buf);
        }

        TEST_METHOD(StreamFixture_readLine_max3)
        {
            char buf[3];

            memset(buf, 'A', sizeof(buf));
            mem.str("ab\r\n");
            Assert::AreEqual(2, mem.readLine(buf, 3));
            Assert::AreEqual("ab", buf);
        }

        TEST_METHOD(StreamFixture_readWord_nullcase)
        {
            char buf[1024] = "ABC";

            mem.str("");
            Assert::AreEqual(0, mem.readWord(buf, 1024));
            Assert::AreEqual("", buf);
        }

        TEST_METHOD(StreamFixture_readWord)
        {
            char buf[1024];

            memset(buf, 0, 1024);
            mem.str("abc def");
            Assert::AreEqual(3, mem.readWord(buf, 1024));
            Assert::AreEqual("abc", buf);
            Assert::AreEqual(3, mem.readWord(buf, 1024));
            Assert::AreEqual("def", buf);

            memset(buf, 0, 1024);
            mem.str("abc\tdef");
            Assert::AreEqual(3, mem.readWord(buf, 1024));
            Assert::AreEqual("abc", buf);
            Assert::AreEqual(3, mem.readWord(buf, 1024));
            Assert::AreEqual("def", buf);

            memset(buf, 0, 1024);
            mem.str("abc\rdef");
            Assert::AreEqual(3, mem.readWord(buf, 1024));
            Assert::AreEqual("abc", buf);
            Assert::AreEqual(3, mem.readWord(buf, 1024));
            Assert::AreEqual("def", buf);

            memset(buf, 0, 1024);
            mem.str("abc\ndef");
            Assert::AreEqual(3, mem.readWord(buf, 1024));
            Assert::AreEqual("abc", buf);
            Assert::AreEqual(3, mem.readWord(buf, 1024));
            Assert::AreEqual("def", buf);
        }

        TEST_METHOD(StreamFixture_readWord_bufferSize)
        {
            char buf[1024] = "ABCD";

            // バッファーサイズが足りないと記録できなかった1文字は消えてしまう
            mem.str("abc");
            Assert::AreEqual(1, mem.readWord(buf, 2));
            Assert::AreEqual("a", buf);
            Assert::AreEqual(1, mem.readWord(buf, 1024));
            Assert::AreEqual("c", buf);
        }

        // このメソッド使い方わからないし、使われてないから消したいな。
        TEST_METHOD(StreamFixture_readBase64)
        {
            // Base64.strict_encode64("foo")
            // => "Zm9v"

            mem.str("Zm9v");
            char buf[1024] = "AAAAAAAAA";
            Assert::AreEqual(3, mem.readBase64(buf, 5));
            Assert::AreEqual("foo", buf);
        }

        TEST_METHOD(StreamFixture_write_vargs)
        {
            StreamFixture::forward_write_vargs(mem, "hello %s", "world");
            Assert::AreEqual("hello world", mem.str().c_str());
            mem.str("");
            StreamFixture::forward_write_vargs(mem, "hello %d %d %d", 1, 2, 3);
            Assert::AreEqual("hello 1 2 3", mem.str().c_str());
        }

        TEST_METHOD(StreamFixture_writeLine)
        {
            Assert::IsTrue(mem.writeCRLF);

            mem.writeLine("abc");
            Assert::AreEqual("abc\r\n", mem.str().c_str());

            mem.str("");
            mem.writeCRLF = false;
            mem.writeLine("abc");
            Assert::AreEqual("abc\n", mem.str().c_str());
        }

        TEST_METHOD(StreamFixture_writeLineF)
        {
            Assert::IsTrue(mem.writeCRLF);

            mem.writeLineF("hello %s", "world");
            Assert::AreEqual("hello world\r\n", mem.str().c_str());

            mem.str("");
            mem.writeCRLF = false;
            mem.writeLineF("hello %s", "world");
            Assert::AreEqual("hello world\n", mem.str().c_str());
        }

        TEST_METHOD(StreamFixture_writeString)
        {
            mem.writeString("hello world");
            Assert::AreEqual("hello world", mem.str().c_str());

            mem.str("");
            mem.writeString(std::string("hello world"));
            Assert::AreEqual("hello world", mem.str().c_str());

            mem.str("");
            mem.writeString(String("hello world"));
            Assert::AreEqual("hello world", mem.str().c_str());
        }

        TEST_METHOD(StreamFixture_writeStringF)
        {
            mem.writeStringF("hello %s", "world");
            Assert::AreEqual("hello world", mem.str().c_str());
        }

        TEST_METHOD(StreamFixture_readBits_nibbles)
        {
            mem.str("\xde\xad\xbe\xef");
            Assert::AreEqual(0xd, mem.readBits(4));
            Assert::AreEqual(0xe, mem.readBits(4));
            Assert::AreEqual(0xa, mem.readBits(4));
            Assert::AreEqual(0xd, mem.readBits(4));
            Assert::AreEqual(0xb, mem.readBits(4));
            Assert::AreEqual(0xe, mem.readBits(4));
            Assert::AreEqual(0xe, mem.readBits(4));
            Assert::AreEqual(0xf, mem.readBits(4));
        }

        TEST_METHOD(StreamFixture_readBits_bits)
        {
            mem.str("\xaa");
            Assert::AreEqual(1, mem.readBits(1));
            Assert::AreEqual(0, mem.readBits(1));
            Assert::AreEqual(1, mem.readBits(1));
            Assert::AreEqual(0, mem.readBits(1));
            Assert::AreEqual(1, mem.readBits(1));
            Assert::AreEqual(0, mem.readBits(1));
            Assert::AreEqual(1, mem.readBits(1));
            Assert::AreEqual(0, mem.readBits(1));
        }

        TEST_METHOD(StreamFixture_readBits_bytes)
        {
            mem.str("\xaa");
            Assert::AreEqual(0xaa, mem.readBits(8));
        }

        TEST_METHOD(StreamFixture_readBits_boundary)
        {
            mem.str("\xde\xad");
            Assert::AreEqual(0xd, mem.readBits(4));
            Assert::AreEqual(0xea, mem.readBits(8));
            Assert::AreEqual(0xd, mem.readBits(4));
        }

        TEST_METHOD(StreamFixture_readBits_4bytes)
        {
            mem.str("\xde\xad\xbe\xef");
            Assert::AreEqual(0xdeadbeef, mem.readBits(32));
        }

        TEST_METHOD(StreamFixture_totalBytesIn)
        {
            Assert::AreEqual(0, s.totalBytesIn());
            s.updateTotals(1, 0);
            Assert::AreEqual(1, s.totalBytesIn());

            s.updateTotals(1, 0);
            Assert::AreEqual(2, s.totalBytesIn());
        }

        TEST_METHOD(StreamFixture_totalBytesOut)
        {
            Assert::AreEqual(0, s.totalBytesOut());
            s.updateTotals(0, 1);
            Assert::AreEqual(1, s.totalBytesOut());

            s.updateTotals(0, 1);
            Assert::AreEqual(2, s.totalBytesOut());
        }

        // 時間を進めないと変化しない。
        TEST_METHOD(StreamFixture_lastBytesIn)
        {
            Assert::AreEqual(0, s.lastBytesIn());
            s.updateTotals(1, 0);
            Assert::AreEqual(0, s.lastBytesIn());
        }

        // 時間を進めないと変化しない。
        TEST_METHOD(StreamFixture_lastBytesOut)
        {
            Assert::AreEqual(0, s.lastBytesOut());
            s.updateTotals(0, 1);
            Assert::AreEqual(0, s.lastBytesOut());
        }

    };
}

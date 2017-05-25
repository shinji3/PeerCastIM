#include "stdafx.h"
#include "CppUnitTest.h"

#include "dechunker.h"
#include "sstream.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace DechunkerFixture
{
    TEST_CLASS(DechunkerFixture)
    {
    public:

        TEST_METHOD(DechunkerFixture_hexValue)
        {
            Assert::AreEqual(0, Dechunker::hexValue('0'));
            Assert::AreEqual(9, Dechunker::hexValue('9'));
            Assert::AreEqual(0xa, Dechunker::hexValue('a'));
            Assert::AreEqual(0xf, Dechunker::hexValue('f'));
            Assert::AreEqual(0xa, Dechunker::hexValue('A'));
            Assert::AreEqual(0xf, Dechunker::hexValue('F'));
            Assert::AreEqual(-1, Dechunker::hexValue('g'));
            Assert::AreEqual(-1, Dechunker::hexValue('G'));
        }

        TEST_METHOD(DechunkerFixture_readChar)
        {
            StringStream mem;

            mem.writeString("4\r\n"
                "Wiki\r\n"
                "5\r\n"
                "pedia\r\n"
                "E\r\n"
                " in\r\n"
                "\r\n"
                "chunks.\r\n"
                "0\r\n"
                "\r\n");
            mem.rewind();

            Dechunker dechunker(mem);

            Assert::AreEqual('W', dechunker.readChar());
            Assert::AreEqual('i', dechunker.readChar());
            Assert::AreEqual('k', dechunker.readChar());
            Assert::AreEqual('i', dechunker.readChar());
            Assert::AreEqual('p', dechunker.readChar());
            Assert::AreEqual('e', dechunker.readChar());
            Assert::AreEqual('d', dechunker.readChar());
            Assert::AreEqual('i', dechunker.readChar());
            Assert::AreEqual('a', dechunker.readChar());
            Assert::AreEqual(' ', dechunker.readChar());
            Assert::AreEqual('i', dechunker.readChar());
            Assert::AreEqual('n', dechunker.readChar());
            Assert::AreEqual('\r', dechunker.readChar());
            Assert::AreEqual('\n', dechunker.readChar());
            Assert::AreEqual('\r', dechunker.readChar());
            Assert::AreEqual('\n', dechunker.readChar());
            Assert::AreEqual('c', dechunker.readChar());
            Assert::AreEqual('h', dechunker.readChar());
            Assert::AreEqual('u', dechunker.readChar());
            Assert::AreEqual('n', dechunker.readChar());
            Assert::AreEqual('k', dechunker.readChar());
            Assert::AreEqual('s', dechunker.readChar());
            Assert::AreEqual('.', dechunker.readChar());
            bool e = false;
            try
            {
                dechunker.readChar();
            }
            catch (StreamException)
            {
                e = true;
            }
            Assert::AreEqual(e, true);
        }

        TEST_METHOD(DechunkerFixture_read)
        {
            StringStream mem;

            mem.writeString("4\r\n"
                "Wiki\r\n"
                "5\r\n"
                "pedia\r\n"
                "E\r\n"
                " in\r\n"
                "\r\n"
                "chunks.\r\n"
                "0\r\n"
                "\r\n");
            mem.rewind();

            Dechunker dechunker(mem);

            char buf[24] = "";
            Assert::AreEqual(23, dechunker.read(buf, 23));
            Assert::AreEqual("Wikipedia in\r\n\r\nchunks.", buf);
        }

    };
}

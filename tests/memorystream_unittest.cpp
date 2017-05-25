#include "stdafx.h"
#include "CppUnitTest.h"

#include "stream.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MemoryStreamFixture
{
    TEST_CLASS(MemoryStreamFixture)
    {
    public:

        MemoryStreamFixture()
            : one_byte_mm(1),
            hoge_mm(data, 4)
        {
            strcpy(data, "hoge");
            one_byte_mm.write("A", 1);
            one_byte_mm.rewind();
        }

        MemoryStream one_byte_mm;
        char data[5];
        MemoryStream hoge_mm;

        // readUpto は実装されていないので何もせず 0 を返す。
        TEST_METHOD(MemoryStreamFixture_readUpto)
        {
            char buf[1024] = "X";
            int result;

            result = one_byte_mm.readUpto(buf, 1);
            Assert::AreEqual(0, result);
            Assert::AreEqual(buf[0], 'X');
        }

        TEST_METHOD(MemoryStreamFixture_PositionAdvancesOnWrite)
        {
            Assert::AreEqual(1, one_byte_mm.len);
            Assert::AreEqual(0, one_byte_mm.getPosition());

            one_byte_mm.write("X", 1);

            Assert::AreEqual(1, one_byte_mm.len);
            Assert::AreEqual(1, one_byte_mm.getPosition());
        }

        TEST_METHOD(MemoryStreamFixture_ThrowsExceptionIfCannotWrite)
        {
            char buf[1024];

            // メモリーに収まらない write は StreamException を上げる。
            bool e = false;
            try
            {
                one_byte_mm.write("XXX", 3);
            }
            catch (StreamException)
            {
                e = true;
            }
            Assert::AreEqual(e, true);

            one_byte_mm.rewind();

            // エラーになった場合は1文字も書き込まれていない。
            one_byte_mm.read(buf, 1);
            Assert::AreEqual('A', buf[0]);
        }


        TEST_METHOD(MemoryStreamFixture_ExternalMemory)
        {
            // Assert::IsFalse(hoge_mm.own);
            Assert::AreEqual(4, hoge_mm.len);
            Assert::AreEqual(0, hoge_mm.getPosition());
        }


        TEST_METHOD(MemoryStreamFixture_read)
        {
            char buf[1024];
            Assert::AreEqual(4, hoge_mm.read(buf, 4));
            Assert::AreEqual(0, strncmp(buf, "hoge", 4));
        }

        TEST_METHOD(MemoryStreamFixture_seekAndRewind)
        {
            Assert::AreEqual(0, hoge_mm.getPosition());
            hoge_mm.seekTo(4);
            Assert::AreEqual(4, hoge_mm.getPosition());
            hoge_mm.rewind();
            Assert::AreEqual(0, hoge_mm.getPosition());
        }

        TEST_METHOD(MemoryStreamFixture_write)
        {
            char buf[1024];

            hoge_mm.write("fuga", 4);
            hoge_mm.rewind();
            hoge_mm.read(buf, 4);
            Assert::AreEqual(0, strncmp(buf, "fuga", 4));
        }

    };
}

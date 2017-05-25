#include "stdafx.h"
#include "CppUnitTest.h"

#include "sstream.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MemoryStreamFixture
{
    TEST_CLASS(MemoryStreamFixture)
    {
    public:

        TEST_METHOD(StringStreamFixture_initialState)
        {
            StringStream s;

            Assert::AreEqual(0, s.getPosition());
            Assert::AreEqual(0, s.getLength());

            Assert::IsTrue(s.eof());

            bool e = false;
            try
            {
                s.read(NULL, 0);
            }
            catch (StreamException)
            {
                e = true;
            }
            Assert::AreEqual(e, true);
        }

        TEST_METHOD(StringStreamFixture_writeAdvancesPositionAndLength)
        {
            StringStream s;

            s.write("hoge", 4);
            Assert::AreEqual(4, s.getPosition());
            Assert::AreEqual(4, s.getLength());
        }

        TEST_METHOD(StringStreamFixture_writeAdvancesPositionAndLengthNullCase)
        {
            StringStream s;

            s.write("", 0);
            Assert::AreEqual(0, s.getPosition());
            Assert::AreEqual(0, s.getLength());
        }

        TEST_METHOD(StringStreamFixture_rewindResetsPosition)
        {
            StringStream s;

            s.write("hoge", 4);
            Assert::AreEqual(4, s.getPosition());
            Assert::AreEqual(true, s.eof());
            s.rewind();
            Assert::AreEqual(0, s.getPosition());
            Assert::AreEqual(false, s.eof());
        }

        TEST_METHOD(StringStreamFixture_whatIsWrittenCanBeRead)
        {
            StringStream s;

            char buf[5] = "";

            s.write("hoge", 4);

            bool e = false;
            try
            {
                s.read(buf, 4);
            }
            catch (StreamException)
            {
                e = true;
            }
            Assert::AreEqual(e, true);

            s.rewind();
            Assert::AreEqual(4, s.read(buf, 100));
            Assert::AreEqual("hoge", buf);
        }

        TEST_METHOD(StringStreamFixture_seekToChangesPosition)
        {
            StringStream s;

            s.write("hoge", 4);
            s.seekTo(1);
            Assert::AreEqual(1, s.getPosition());

            char buf[4] = "";
            s.read(buf, 3);
            Assert::AreEqual("oge", buf);
        }

        TEST_METHOD(StringStreamFixture_seekToChangesLength)
        {
            StringStream s;

            s.seekTo(1000);
            Assert::AreEqual(1000, s.getLength());

            char buf[1000];
            buf[0] = (char)0xff;
            buf[999] = (char)0xff;

            s.rewind();
            s.read(buf, 1000);
            Assert::AreEqual(0, (int)buf[0]);
            Assert::AreEqual(0, (int)buf[999]);
        }

        TEST_METHOD(StringStreamFixture_strGet)
        {
            StringStream s;

            s.writeString("hoge");
            Assert::AreEqual(4, s.getPosition());
            Assert::AreEqual(4, s.getLength());

            Assert::AreEqual("hoge", s.str().c_str());
            Assert::AreEqual(4, s.getPosition());
            Assert::AreEqual(4, s.getLength());
        }

        TEST_METHOD(StringStreamFixture_strSet)
        {
            StringStream s;

            s.writeString("hoge");
            Assert::AreEqual(4, s.getPosition());
            Assert::AreEqual(4, s.getLength());

            s.str("foo");
            Assert::AreEqual(0, s.getPosition());
            Assert::AreEqual(3, s.getLength());

            Assert::AreEqual("foo", s.str().c_str());
        }

    };
}

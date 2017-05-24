#include "stdafx.h"
#include "CppUnitTest.h"

#include "chandir.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ChannelDirectoryFixture
{
    TEST_CLASS(ChannelDirectoryFixture)
    {
    public:

        ChannelDirectory dir;

//      TEST_METHOD(update)
//      {
//          bool res = dir.update();

//          Assert::AreEqual(false, res);
//      }

        TEST_METHOD(findTracker)
        {
            ChannelEntry entry({ "", "01234567890123456789012345678901", "127.0.0.1:7144", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "" }, "http://example.com/index.txt");

            dir.m_channels.push_back(entry);
            GnuID id("01234567890123456789012345678901");
            Assert::AreEqual("127.0.0.1:7144", dir.findTracker(id).c_str());

            GnuID id2("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
            Assert::AreEqual("", dir.findTracker(id2).c_str());
        }

    };
}

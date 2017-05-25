#include "stdafx.h"
#include "CppUnitTest.h"

#include "chandir.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ChannelEntryFixture
{
    TEST_CLASS(ChannelEntryFixture)
    {
    public:

        TEST_METHOD(ChannelEntryFixture_constructor)
        {
            bool e = false;
            try
            {
                ChannelEntry({}, "");
            }
            catch (std::runtime_error)
            {
                e = true;
            }
            Assert::AreEqual(e, true);
        }

        TEST_METHOD(ChannelEntryFixture_textToChannelEntries)
        {
            auto vec = ChannelEntry::textToChannelEntries("予定地<>97968780D09CC97BB98D4A2BF221EDE7<>127.0.0.1:7144<>http://www.example.com/<>プログラミング<>peercastをいじる - &lt;Free&gt;<>-1<>-1<>428<>FLV<><><><><>%E4%BA%88%E5%AE%9A%E5%9C%B0<>1:14<>click<><>1\n", "");

            Assert::AreEqual(1, (int)vec.size());

            auto& entry = vec[0];

            Assert::AreEqual("予定地", entry.name.c_str());
            Assert::AreEqual("97968780D09CC97BB98D4A2BF221EDE7", ((std::string) entry.id).c_str());
            Assert::AreEqual(428, entry.bitrate);
            Assert::AreEqual("FLV", entry.contentTypeStr.c_str());
            Assert::AreEqual("peercastをいじる - &lt;Free&gt;", entry.desc.c_str());
            Assert::AreEqual("プログラミング", entry.genre.c_str());
            Assert::AreEqual("http://www.example.com/", entry.url.c_str());
            Assert::AreEqual("127.0.0.1:7144", entry.tip.c_str());
            Assert::AreEqual("1:14", entry.uptime.c_str());
            Assert::AreEqual("%E4%BA%88%E5%AE%9A%E5%9C%B0", entry.encodedName.c_str());
            Assert::AreEqual(-1, entry.numDirects);
            Assert::AreEqual(-1, entry.numRelays);
        }


    };
}

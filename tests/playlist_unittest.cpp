#include "stdafx.h"
#include "CppUnitTest.h"

#include "playlist.h"
#include "sstream.h"

#include "mockpeercast.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PlayListFixture
{
    TEST_CLASS(PlayListFixture)
    {
    public:

        PlayListFixture()
            : pls(PlayList::T_PLS, 1)
            , asx(PlayList::T_ASX, 1)
        {
            peercastApp = new MockPeercastApplication();
            peercastInst = new MockPeercastInstance();
            peercastInst->init();
        }

        PlayList pls;
        PlayList asx;

        TEST_METHOD(PlayListFixture_initialState)
        {
            Assert::AreEqual((int)PlayList::T_PLS, (int)pls.type);
            Assert::AreEqual(0, pls.numURLs);
            Assert::AreEqual(1, pls.maxURLs);
            Assert::IsNotNull(pls.urls);
            Assert::IsNotNull(pls.titles);

            Assert::AreEqual((int)PlayList::T_ASX, (int)asx.type);
            Assert::AreEqual(0, asx.numURLs);
            Assert::AreEqual(1, asx.maxURLs);
            Assert::IsNotNull(asx.urls);
            Assert::IsNotNull(asx.titles);
        }

        TEST_METHOD(PlayListFixture_addURL)
        {
            Assert::AreEqual(0, pls.numURLs);

            pls.addURL("http://127.0.0.1:7144/stream/00000000000000000000000000000000.flv", "A ch", "http://jbbs.shitaraba.net/bbs/read.cgi/internet/7144/0000000000/");

            Assert::AreEqual(1, pls.numURLs);
            Assert::AreEqual("http://127.0.0.1:7144/stream/00000000000000000000000000000000.flv", pls.urls[0].cstr());
            Assert::AreEqual("A ch", pls.titles[0].cstr());
            Assert::AreEqual("http://jbbs.shitaraba.net/bbs/read.cgi/internet/7144/0000000000/", pls.contacturls[0].cstr());

            // cannot add beyond maxURLs
            pls.addURL("http://127.0.0.1:7144/stream/FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF.flv", "B ch", "http://jbbs.shitaraba.net/bbs/read.cgi/internet/7144/FFFFFFFFFF/");

            Assert::AreEqual(1, pls.numURLs);
        }

        TEST_METHOD(PlayListFixture_addChannel)
        {
            ChanInfo info;
            info.name = "1ch";
            info.id.fromStr("01234567890123456789012345678901");
            info.contentType = ChanInfo::T_FLV;
            pls.addChannel("http://127.0.0.1:7144", info);
            Assert::AreEqual(1, pls.numURLs);
            Assert::AreEqual("http://127.0.0.1:7144/stream/01234567890123456789012345678901.flv?auth=44d5299e57ad9274fee7960a9fa60bfd", pls.urls[0].cstr());
            Assert::AreEqual("1ch", pls.titles[0].cstr());

            info.contentType = ChanInfo::T_WMV;
            asx.addChannel("http://127.0.0.1:7144", info);
            Assert::AreEqual("http://127.0.0.1:7144/stream/01234567890123456789012345678901.wmv?auth=44d5299e57ad9274fee7960a9fa60bfd", asx.urls[0].cstr());
        }

        TEST_METHOD(PlayListFixture_write_pls)
        {
            StringStream mem;

            ChanInfo info;
            info.name = "1ch";
            info.id.fromStr("01234567890123456789012345678901");
            info.contentType = ChanInfo::T_FLV;
            pls.addChannel("http://127.0.0.1:7144", info);

            pls.write(mem);
            Assert::AreEqual("http://127.0.0.1:7144/stream/01234567890123456789012345678901.flv?auth=44d5299e57ad9274fee7960a9fa60bfd\r\n",
                mem.str().c_str());
        }

        TEST_METHOD(PlayListFixture_write_asx)
        {
            StringStream mem;

            ChanInfo info;
            info.name = "1ch";
            info.id.fromStr("01234567890123456789012345678901");
            info.contentType = ChanInfo::T_WMV;
            asx.addChannel("http://127.0.0.1:7144", info);

            asx.write(mem);
            Assert::AreEqual("<ASX Version=\"3.0\">\r\n"
                "<TITLE>1ch</TITLE>\r\n"
                "<ENTRY>\r\n"
                "<TITLE>1ch</TITLE>\r\n"
                "<REF href = \"http://127.0.0.1:7144/stream/01234567890123456789012345678901.wmv?auth=44d5299e57ad9274fee7960a9fa60bfd\" />\r\n"
                "</ENTRY>\r\n"
                "</ASX>\r\n",
                mem.str().c_str());
        }

        TEST_METHOD(PlayListFixture_getPlayListType)
        {
            Assert::AreEqual((int)PlayList::T_ASX, (int)PlayList::getPlayListType(ChanInfo::T_WMA));
            Assert::AreEqual((int)PlayList::T_ASX, (int)PlayList::getPlayListType(ChanInfo::T_WMV));
            Assert::AreEqual((int)PlayList::T_RAM, (int)PlayList::getPlayListType(ChanInfo::T_OGM));
            Assert::AreEqual((int)PlayList::T_PLS, (int)PlayList::getPlayListType(ChanInfo::T_OGG));
            Assert::AreEqual((int)PlayList::T_PLS, (int)PlayList::getPlayListType(ChanInfo::T_MP3));
            Assert::AreEqual((int)PlayList::T_PLS, (int)PlayList::getPlayListType(ChanInfo::T_FLV));
            Assert::AreEqual((int)PlayList::T_PLS, (int)PlayList::getPlayListType(ChanInfo::T_MKV));
        }

    };
}

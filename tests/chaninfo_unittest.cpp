#include "stdafx.h"
#include "CppUnitTest.h"

#include "channel.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ChanInfoFixture
{
    TEST_CLASS(ChanInfoFixture)
    {
    public:

        ChanInfo info;

        TEST_METHOD(ChanInfoFixture_initialState)
        {
            Assert::AreEqual("", info.name.cstr());
            Assert::AreEqual("00000000000000000000000000000000", static_cast<std::string>(info.id).c_str());
            Assert::AreEqual("00000000000000000000000000000000", static_cast<std::string>(info.bcID).c_str());
            Assert::AreEqual(0, info.bitrate);
            Assert::AreEqual((int)ChanInfo::T_UNKNOWN, (int)info.contentType);
            Assert::AreEqual("", info.contentTypeStr.cstr());
            Assert::AreEqual("", info.MIMEType.cstr());
            Assert::AreEqual("", info.streamExt.cstr());
            Assert::AreEqual((int)ChanInfo::PROTOCOL::SP_UNKNOWN, (int)info.srcProtocol);
            Assert::AreEqual(0, (int)info.lastPlayStart);
            Assert::AreEqual(0, (int)info.lastPlayEnd);
            Assert::AreEqual(0, (int)info.numSkips);
            Assert::AreEqual(0, (int)info.createdTime);
            Assert::AreEqual((int)ChanInfo::S_UNKNOWN, (int)info.status);

            {
                Assert::AreEqual("", info.track.contact.cstr());
                Assert::AreEqual("", info.track.title.cstr());
                Assert::AreEqual("", info.track.artist.cstr());
                Assert::AreEqual("", info.track.album.cstr());
                Assert::AreEqual("", info.track.genre.cstr());
            }

            Assert::AreEqual("", info.desc);
            Assert::AreEqual("", info.genre);
            Assert::AreEqual("", info.url);
            Assert::AreEqual("", info.comment);
        }

        TEST_METHOD(ChanInfoFixture_writeInfoAtoms)
        {
            MemoryStream mem(1024);
            AtomStream atom(mem);

            info.writeInfoAtoms(atom);

            Assert::AreEqual(81, mem.getPosition());
        }

        TEST_METHOD(ChanInfoFixture_writeTrackAtoms)
        {
            MemoryStream mem(1024);
            AtomStream atom(mem);

            info.writeTrackAtoms(atom);

            Assert::AreEqual(44, mem.getPosition());
        }

        TEST_METHOD(ChanInfoFixture_getters)
        {
            Assert::AreEqual(0, (int)info.getUptime());
            Assert::AreEqual(0, (int)info.getAge());
            Assert::AreEqual(false, info.isActive());
            Assert::AreEqual(false, info.isPrivate());
            Assert::AreEqual("UNKNOWN", info.getTypeStr());
            Assert::AreEqual("", info.getTypeExt());
            Assert::AreEqual("application/octet-stream", info.getMIMEType());
        }

        TEST_METHOD(ChanInfoFixture_static_getTypeStr)
        {
            Assert::AreEqual("RAW", ChanInfo::getTypeStr(ChanInfo::T_RAW));
            Assert::AreEqual("MP3", ChanInfo::getTypeStr(ChanInfo::T_MP3));
            Assert::AreEqual("OGG", ChanInfo::getTypeStr(ChanInfo::T_OGG));
            Assert::AreEqual("OGM", ChanInfo::getTypeStr(ChanInfo::T_OGM));
            Assert::AreEqual("WMA", ChanInfo::getTypeStr(ChanInfo::T_WMA));
            Assert::AreEqual("MOV", ChanInfo::getTypeStr(ChanInfo::T_MOV));
            Assert::AreEqual("MPG", ChanInfo::getTypeStr(ChanInfo::T_MPG));
            Assert::AreEqual("NSV", ChanInfo::getTypeStr(ChanInfo::T_NSV));
            Assert::AreEqual("WMV", ChanInfo::getTypeStr(ChanInfo::T_WMV));
            Assert::AreEqual("FLV", ChanInfo::getTypeStr(ChanInfo::T_FLV));
            Assert::AreEqual("PLS", ChanInfo::getTypeStr(ChanInfo::T_PLS));
            Assert::AreEqual("ASX", ChanInfo::getTypeStr(ChanInfo::T_ASX));
            Assert::AreEqual("UNKNOWN", ChanInfo::getTypeStr(ChanInfo::T_UNKNOWN));
        }

        TEST_METHOD(ChanInfoFixture_static_getProtocolStr)
        {
            Assert::AreEqual("PEERCAST", ChanInfo::getProtocolStr(ChanInfo::SP_PEERCAST));
            Assert::AreEqual("HTTP", ChanInfo::getProtocolStr(ChanInfo::SP_HTTP));
            Assert::AreEqual("FILE", ChanInfo::getProtocolStr(ChanInfo::SP_FILE));
            Assert::AreEqual("MMS", ChanInfo::getProtocolStr(ChanInfo::SP_MMS));
            Assert::AreEqual("PCP", ChanInfo::getProtocolStr(ChanInfo::SP_PCP));
            Assert::AreEqual("WMHTTP", ChanInfo::getProtocolStr(ChanInfo::SP_WMHTTP));
            Assert::AreEqual("UNKNOWN", ChanInfo::getProtocolStr(ChanInfo::SP_UNKNOWN));
        }

        TEST_METHOD(ChanInfoFixture_static_getTypeExt)
        {
            Assert::AreEqual(".ogg", ChanInfo::getTypeExt(ChanInfo::T_OGM));
            Assert::AreEqual(".ogg", ChanInfo::getTypeExt(ChanInfo::T_OGG));
            Assert::AreEqual(".mp3", ChanInfo::getTypeExt(ChanInfo::T_MP3));
            Assert::AreEqual(".mov", ChanInfo::getTypeExt(ChanInfo::T_MOV));
            Assert::AreEqual(".nsv", ChanInfo::getTypeExt(ChanInfo::T_NSV));
            Assert::AreEqual(".wmv", ChanInfo::getTypeExt(ChanInfo::T_WMV));
            Assert::AreEqual(".wma", ChanInfo::getTypeExt(ChanInfo::T_WMA));
            Assert::AreEqual(".flv", ChanInfo::getTypeExt(ChanInfo::T_FLV));
            Assert::AreEqual("", ChanInfo::getTypeExt(ChanInfo::T_UNKNOWN));
        }

        TEST_METHOD(ChanInfoFixture_static_getMIMEStr)
        {
            Assert::AreEqual("application/x-ogg", ChanInfo::getMIMEType(ChanInfo::T_OGG));
            Assert::AreEqual("application/x-ogg", ChanInfo::getMIMEType(ChanInfo::T_OGM));
            Assert::AreEqual("audio/mpeg", ChanInfo::getMIMEType(ChanInfo::T_MP3));
            Assert::AreEqual("video/quicktime", ChanInfo::getMIMEType(ChanInfo::T_MOV));
            Assert::AreEqual("video/mpeg", ChanInfo::getMIMEType(ChanInfo::T_MPG));
            Assert::AreEqual("video/nsv", ChanInfo::getMIMEType(ChanInfo::T_NSV));
            Assert::AreEqual("video/x-ms-asf", ChanInfo::getMIMEType(ChanInfo::T_ASX));
            Assert::AreEqual("audio/x-ms-wma", ChanInfo::getMIMEType(ChanInfo::T_WMA));
            Assert::AreEqual("video/x-ms-wmv", ChanInfo::getMIMEType(ChanInfo::T_WMV));
            Assert::AreEqual("video/x-flv", ChanInfo::getMIMEType(ChanInfo::T_FLV));
        }

        TEST_METHOD(ChanInfoFixture_static_getTypeFromStr)
        {
            Assert::AreEqual((int)ChanInfo::T_MP3, (int)ChanInfo::getTypeFromStr("MP3"));
            Assert::AreEqual((int)ChanInfo::T_OGG, (int)ChanInfo::getTypeFromStr("OGG"));
            Assert::AreEqual((int)ChanInfo::T_OGM, (int)ChanInfo::getTypeFromStr("OGM"));
            Assert::AreEqual((int)ChanInfo::T_RAW, (int)ChanInfo::getTypeFromStr("RAW"));
            Assert::AreEqual((int)ChanInfo::T_NSV, (int)ChanInfo::getTypeFromStr("NSV"));
            Assert::AreEqual((int)ChanInfo::T_WMA, (int)ChanInfo::getTypeFromStr("WMA"));
            Assert::AreEqual((int)ChanInfo::T_WMV, (int)ChanInfo::getTypeFromStr("WMV"));
            Assert::AreEqual((int)ChanInfo::T_FLV, (int)ChanInfo::getTypeFromStr("FLV"));
            Assert::AreEqual((int)ChanInfo::T_PLS, (int)ChanInfo::getTypeFromStr("PLS"));
            Assert::AreEqual((int)ChanInfo::T_PLS, (int)ChanInfo::getTypeFromStr("M3U"));
            Assert::AreEqual((int)ChanInfo::T_ASX, (int)ChanInfo::getTypeFromStr("ASX"));

            Assert::AreEqual((int)ChanInfo::T_MP3, (int)ChanInfo::getTypeFromStr("mp3")); // type str. is case-insensitive
            Assert::AreEqual((int)ChanInfo::T_UNKNOWN, (int)ChanInfo::getTypeFromStr("mp345"));
        }

        TEST_METHOD(ChanInfoFixture_static_getProtocolFromStr)
        {
            Assert::AreEqual((int)ChanInfo::SP_PEERCAST, (int)ChanInfo::getProtocolFromStr("PEERCAST"));
            Assert::AreEqual((int)ChanInfo::SP_HTTP, (int)ChanInfo::getProtocolFromStr("HTTP"));
            Assert::AreEqual((int)ChanInfo::SP_FILE, (int)ChanInfo::getProtocolFromStr("FILE"));
            Assert::AreEqual((int)ChanInfo::SP_MMS, (int)ChanInfo::getProtocolFromStr("MMS"));
            Assert::AreEqual((int)ChanInfo::SP_PCP, (int)ChanInfo::getProtocolFromStr("PCP"));
            Assert::AreEqual((int)ChanInfo::SP_WMHTTP, (int)ChanInfo::getProtocolFromStr("WMHTTP"));

            Assert::AreEqual((int)ChanInfo::SP_PEERCAST, (int)ChanInfo::getProtocolFromStr("Peercast")); // type str. is case-insesitive
            Assert::AreEqual((int)ChanInfo::SP_UNKNOWN, (int)ChanInfo::getProtocolFromStr("RTMP"));
        }

        TEST_METHOD(ChanInfoFixture_setContentType)
        {
            Assert::AreEqual((int)ChanInfo::T_UNKNOWN, (int)info.contentType);

            info.setContentType(ChanInfo::T_MKV);
            Assert::AreEqual((int)ChanInfo::T_MKV, (int)info.contentType);
            Assert::AreEqual("MKV", info.contentTypeStr.cstr());
            Assert::AreEqual("video/x-matroska", info.MIMEType.cstr());
            Assert::AreEqual(".mkv", info.streamExt.cstr());
        }

        TEST_METHOD(ChanInfoFixture_getPlayListExt)
        {
            info.setContentType(ChanInfo::T_MP3);
            Assert::AreEqual(".m3u", info.getPlayListExt());

            info.setContentType(ChanInfo::T_OGG);
            Assert::AreEqual(".m3u", info.getPlayListExt());

            info.setContentType(ChanInfo::T_OGM);
            Assert::AreEqual(".ram", info.getPlayListExt());

            info.setContentType(ChanInfo::T_WMV);
            Assert::AreEqual(".asx", info.getPlayListExt());

            info.setContentType(ChanInfo::T_FLV);
            Assert::AreEqual(".m3u", info.getPlayListExt());

            info.setContentType(ChanInfo::T_MKV);
            Assert::AreEqual(".m3u", info.getPlayListExt());
        }

    };
}

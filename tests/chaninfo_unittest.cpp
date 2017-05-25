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
            Assert::AreEqual(ChanInfo::T_UNKNOWN, info.contentType);
            Assert::AreEqual("", info.contentTypeStr.cstr());
            Assert::AreEqual("", info.MIMEType.cstr());
            Assert::AreEqual("", info.streamExt.cstr());
            Assert::AreEqual(ChanInfo::PROTOCOL::SP_UNKNOWN, info.srcProtocol);
            Assert::AreEqual(0, info.lastPlayStart);
            Assert::AreEqual(0, info.lastPlayEnd);
            Assert::AreEqual(0, info.numSkips);
            Assert::AreEqual(0, info.createdTime);
            Assert::AreEqual(ChanInfo::S_UNKNOWN, info.status);

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
            Assert::AreEqual(0, info.getUptime());
            Assert::AreEqual(0, info.getAge());
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
            Assert::AreEqual(ChanInfo::T_MP3, ChanInfo::getTypeFromStr("MP3"));
            Assert::AreEqual(ChanInfo::T_OGG, ChanInfo::getTypeFromStr("OGG"));
            Assert::AreEqual(ChanInfo::T_OGM, ChanInfo::getTypeFromStr("OGM"));
            Assert::AreEqual(ChanInfo::T_RAW, ChanInfo::getTypeFromStr("RAW"));
            Assert::AreEqual(ChanInfo::T_NSV, ChanInfo::getTypeFromStr("NSV"));
            Assert::AreEqual(ChanInfo::T_WMA, ChanInfo::getTypeFromStr("WMA"));
            Assert::AreEqual(ChanInfo::T_WMV, ChanInfo::getTypeFromStr("WMV"));
            Assert::AreEqual(ChanInfo::T_FLV, ChanInfo::getTypeFromStr("FLV"));
            Assert::AreEqual(ChanInfo::T_PLS, ChanInfo::getTypeFromStr("PLS"));
            Assert::AreEqual(ChanInfo::T_PLS, ChanInfo::getTypeFromStr("M3U"));
            Assert::AreEqual(ChanInfo::T_ASX, ChanInfo::getTypeFromStr("ASX"));

            Assert::AreEqual(ChanInfo::T_MP3, ChanInfo::getTypeFromStr("mp3")); // type str. is case-insensitive
            Assert::AreEqual(ChanInfo::T_UNKNOWN, ChanInfo::getTypeFromStr("mp345"));
        }

        TEST_METHOD(ChanInfoFixture_static_getProtocolFromStr)
        {
            Assert::AreEqual(ChanInfo::SP_PEERCAST, ChanInfo::getProtocolFromStr("PEERCAST"));
            Assert::AreEqual(ChanInfo::SP_HTTP, ChanInfo::getProtocolFromStr("HTTP"));
            Assert::AreEqual(ChanInfo::SP_FILE, ChanInfo::getProtocolFromStr("FILE"));
            Assert::AreEqual(ChanInfo::SP_MMS, ChanInfo::getProtocolFromStr("MMS"));
            Assert::AreEqual(ChanInfo::SP_PCP, ChanInfo::getProtocolFromStr("PCP"));
            Assert::AreEqual(ChanInfo::SP_WMHTTP, ChanInfo::getProtocolFromStr("WMHTTP"));

            Assert::AreEqual(ChanInfo::SP_PEERCAST, ChanInfo::getProtocolFromStr("Peercast")); // type str. is case-insesitive
            Assert::AreEqual(ChanInfo::SP_UNKNOWN, ChanInfo::getProtocolFromStr("RTMP"));
        }

        TEST_METHOD(ChanInfoFixture_setContentType)
        {
            Assert::AreEqual(ChanInfo::T_UNKNOWN, info.contentType);

            info.setContentType(ChanInfo::T_MKV);
            Assert::AreEqual(ChanInfo::T_MKV, info.contentType);
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

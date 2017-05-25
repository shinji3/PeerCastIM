#include "stdafx.h"
#include "CppUnitTest.h"

#include "channel.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ChannelFixture
{
    TEST_CLASS(ChannelFixture)
    {
    public:

        TEST_METHOD(ChannelFixture_initialState)
        {
            Channel c;

            // ::String            mount;
            Assert::AreEqual("", c.mount.cstr());
            // ChanMeta            insertMeta;
            // ChanPacket          headPack;

            // ChanPacketBuffer    rawData;

            // ChannelStream       *sourceStream;
            Assert::IsNull(c.sourceStream);
            // unsigned int        streamIndex;
            Assert::AreEqual(0, (int)c.streamIndex);

            // ChanInfo            info;
            // ChanHit             sourceHost;
            // ChanHit             designatedHost;

            // GnuID               remoteID;
            Assert::IsFalse(c.remoteID.isSet());

            // ::String            sourceURL;
            Assert::AreEqual("", c.sourceURL.cstr());

            // bool                bump, stayConnected;
            Assert::IsFalse(c.bump);
            Assert::IsFalse(c.stayConnected);
            // int                 icyMetaInterval;
            Assert::AreEqual(0, c.icyMetaInterval);
            // unsigned int        streamPos;
            Assert::AreEqual(0, (int)c.streamPos);
            // bool                readDelay;
            Assert::IsFalse(c.readDelay);

            // TYPE                type;
            Assert::AreEqual(Channel::T_NONE, c.type);
            // ChannelSource       *sourceData;
            Assert::IsNull(c.sourceData);

            // SRC_TYPE            srcType;
            Assert::AreEqual(Channel::SRC_NONE, c.srcType);

            // MP3Header           mp3Head;
            // ThreadInfo          thread;

            // unsigned int        lastIdleTime;
            Assert::AreEqual(0, (int)c.lastIdleTime);
            // STATUS              status;
            Assert::AreEqual(Channel::S_NONE, c.status);

            // ClientSocket        *sock;
            Assert::IsNull(c.sock);
            // ClientSocket        *pushSock;
            Assert::IsNull(c.pushSock);

            // unsigned int        lastTrackerUpdate;
            Assert::AreEqual(0, (int)c.lastTrackerUpdate);
            // unsigned int        lastMetaUpdate;
            Assert::AreEqual(0, (int)c.lastMetaUpdate);

            // double              startTime, syncTime;
            Assert::AreEqual(0, (int)c.startTime);
            Assert::AreEqual(0, (int)c.syncTime);

            // WEvent              syncEvent;

            // Channel             *next;
            Assert::IsNull(c.next);
        }

        TEST_METHOD(ChannelFixture_renderHexDump)
        {
            Assert::AreEqual("",
                Channel::renderHexDump("").c_str());
            Assert::AreEqual("41                                               A\n",
                Channel::renderHexDump("A").c_str());
            Assert::AreEqual("41 41 41 41 41 41 41 41 41 41 41 41 41 41 41 41  AAAAAAAAAAAAAAAA\n",
                Channel::renderHexDump("AAAAAAAAAAAAAAAA").c_str());
            Assert::AreEqual("41 41 41 41 41 41 41 41 41 41 41 41 41 41 41 41  AAAAAAAAAAAAAAAA\n"
                "41                                               A\n",
                Channel::renderHexDump("AAAAAAAAAAAAAAAAA").c_str());
        }

    };
}

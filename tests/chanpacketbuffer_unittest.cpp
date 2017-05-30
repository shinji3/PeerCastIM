#include "stdafx.h"
#include "CppUnitTest.h"

#include "mockpeercast.h"
#include "cstream.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ChanPacketBufferFixture
{
    TEST_CLASS(ChanPacketBufferFixture)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            peercastApp = new MockPeercastApplication();
            peercastInst = new MockPeercastInstance();
            peercastInst->init();
        }

        ChanPacketBuffer data;

        TEST_METHOD(ChanPacketBufferFixture_init)
        {
            data.init();

            Assert::AreEqual(0, (int)data.lastPos);
            Assert::AreEqual(0, (int)data.firstPos);
            Assert::AreEqual(0, (int)data.safePos);
            Assert::AreEqual(0, (int)data.readPos);
            Assert::AreEqual(0, (int)data.writePos);
            Assert::AreEqual(0, (int)data.accept);
            Assert::AreEqual(0, (int)data.lastWriteTime);

            Assert::AreEqual(0, data.numPending());
        }

        TEST_METHOD(ChanPacketBufferFixture_addPacket)
        {
            ChanPacket packet;

            packet.type = ChanPacket::T_DATA;
            packet.len = 8192;
            packet.pos = 0;

            data.writePacket(packet);

            Assert::AreEqual(0, (int)data.lastPos);
            Assert::AreEqual(0, (int)data.firstPos);
            Assert::AreEqual(0, (int)data.safePos);
            Assert::AreEqual(0, (int)data.readPos);
            Assert::AreEqual(1, (int)data.writePos);
            Assert::AreEqual(0, (int)data.accept);
            Assert::AreEqual(0, (int)data.lastWriteTime);
            Assert::AreEqual(1, data.numPending());
        }

    };
}

#include "stdafx.h"
#include "CppUnitTest.h"

#include "cstream.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ChanPacketFixture
{
    TEST_CLASS(ChanPacketFixture)
    {
    public:

        ChanPacket p;

        TEST_METHOD(ChanPacketFixture_initialState)
        {
            Assert::AreEqual((int)ChanPacket::T_UNKNOWN, (int)p.type);
            Assert::AreEqual(0, (int)p.len);
            Assert::AreEqual(0, (int)p.pos);
            Assert::AreEqual(0, (int)p.sync);

            // we don't know what's in p.data[]
        }

        TEST_METHOD(ChanPacketFixture_init)
        {
            p.type = ChanPacket::T_DATA;
            p.len = 1;
            p.pos = 2;
            p.sync = 3;

            p.init();

            Assert::AreEqual((int)ChanPacket::T_UNKNOWN, (int)p.type);
            Assert::AreEqual(0, (int)p.len);
            Assert::AreEqual(0, (int)p.pos);
            Assert::AreEqual(0, (int)p.sync);
        }

    };
}

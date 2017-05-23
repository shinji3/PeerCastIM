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

        TEST_METHOD(initialState)
        {
            Assert::AreEqual(ChanPacket::T_UNKNOWN, p.type);
            Assert::AreEqual(0, p.len);
            Assert::AreEqual(0, p.pos);
            Assert::AreEqual(0, p.sync);

            // we don't know what's in p.data[]
        }

        TEST_METHOD(init)
        {
            p.type = ChanPacket::T_DATA;
            p.len = 1;
            p.pos = 2;
            p.sync = 3;

            p.init();

            Assert::AreEqual(ChanPacket::T_UNKNOWN, p.type);
            Assert::AreEqual(0, p.len);
            Assert::AreEqual(0, p.pos);
            Assert::AreEqual(0, p.sync);
        }

    };
}

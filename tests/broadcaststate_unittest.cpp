#include "stdafx.h"
#include "CppUnitTest.h"

#include "pcp.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BroadcastStateFixture
{
    TEST_CLASS(BroadcastStateFixture)
    {
    public:

        BroadcastState bcs;

        TEST_METHOD(BroadcastStateFixture_initialState)
        {
            Assert::IsFalse(bcs.chanID.isSet());
            Assert::IsFalse(bcs.bcID.isSet());
            Assert::AreEqual(0, bcs.numHops);
            Assert::IsFalse(bcs.forMe);
            Assert::AreEqual(0, (int)bcs.streamPos);
            Assert::AreEqual(0, bcs.group);
        }

        TEST_METHOD(BroadcastStateFixture_initPacketSettings)
        {
            bcs.forMe = true;
            bcs.group = 1;
            bcs.numHops = 5;
            bcs.bcID.fromStr("DEADBEEFDEADBEEFDEADBEEFDEADBEEF");
            bcs.chanID.fromStr("DEADBEEFDEADBEEFDEADBEEFDEADBEEF");
            bcs.streamPos = 1234;

            bcs.initPacketSettings();

            Assert::IsFalse(bcs.chanID.isSet());
            Assert::IsFalse(bcs.bcID.isSet());
            Assert::AreEqual(0, bcs.numHops);
            Assert::IsFalse(bcs.forMe);
            Assert::AreEqual(1234, (int)bcs.streamPos); // Ç±ÇÍÇÕÉNÉäÉAÇ≥ÇÍÇ»Ç¢ÅB
            Assert::AreEqual(0, bcs.group);
        }

    };
}

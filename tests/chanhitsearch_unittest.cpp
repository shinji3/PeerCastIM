#include "stdafx.h"
#include "CppUnitTest.h"

#include "channel.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ChanHitSearchFixture
{
    TEST_CLASS(ChanHitSearchFixture)
    {
    public:

        ChanHitSearch chs;

        TEST_METHOD(ChanHitSearchFixture_initialState)
        {
            char buf[33];
            GnuID zero_id;
            zero_id.clear();

            chs.matchHost.toStr(buf);
            Assert::AreEqual("0.0.0.0:0", buf);

            Assert::AreEqual(0, (int)chs.waitDelay);
            Assert::AreEqual(false, chs.useFirewalled);
            Assert::AreEqual(false, chs.trackersOnly);
            Assert::AreEqual(true, chs.useBusyRelays);
            Assert::AreEqual(true, chs.useBusyControls);
            Assert::IsTrue(chs.excludeID.isSame(zero_id));
            Assert::AreEqual(0, chs.numResults);
        }

    };
}

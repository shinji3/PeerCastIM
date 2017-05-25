#include "stdafx.h"
#include "CppUnitTest.h"

#include "notif.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NotificationFixture
{
    TEST_CLASS(NotificationFixture)
    {
    public:

        TEST_METHOD(NotificationFixture_initialState1)
        {
            Notification notif;
            Assert::AreEqual(0, (int)notif.time);
            Assert::AreEqual((int)ServMgr::NT_PEERCAST, (int)notif.type);
            Assert::AreEqual("", notif.message.c_str());
        }

        TEST_METHOD(NotificationFixture_initialState2)
        {
            Notification notif(1, ServMgr::NT_BROADCASTERS, "A");
            Assert::AreEqual(1, (int)notif.time);
            Assert::AreEqual((int)ServMgr::NT_BROADCASTERS, (int)notif.type);
            Assert::AreEqual("A", notif.message.c_str());
        }

        TEST_METHOD(NotificationFixture_getTypeStr)
        {
            Notification notif;

            notif.type = ServMgr::NT_UPGRADE;
            Assert::AreEqual("Upgrade Alert", notif.getTypeStr().c_str());

            notif.type = ServMgr::NT_PEERCAST;
            Assert::AreEqual("Peercast", notif.getTypeStr().c_str());

            notif.type = ServMgr::NT_BROADCASTERS;
            Assert::AreEqual("Broadcasters", notif.getTypeStr().c_str());

            notif.type = ServMgr::NT_TRACKINFO;
            Assert::AreEqual("Track Info", notif.getTypeStr().c_str());

            notif.type = (ServMgr::NOTIFY_TYPE)7144;
            Assert::AreEqual("Unknown", notif.getTypeStr().c_str());
        }

    };
}

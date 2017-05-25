#include "stdafx.h"
#include "CppUnitTest.h"

#include "notif.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NotificationBufferFixture
{
    TEST_CLASS(NotificationBufferFixture)
    {
    public:

        TEST_METHOD(NotificationBufferFixture_initialState)
        {
            NotificationBuffer buf;

            Assert::AreEqual(0, buf.numNotifications());
            Assert::AreEqual(0, buf.numUnread());

            bool e = false;
            try
            {
                buf.getNotification(123);
            }
            catch (...)
            {
                e = true;
            }
            Assert::AreNotEqual(e, true);

            NotificationBuffer::Entry entry = buf.getNotification(123);
            Assert::IsFalse(entry.isRead);
        }

        TEST_METHOD(NotificationBufferFixture_addNotification)
        {
            NotificationBuffer buf;
            Notification notif;

            Assert::AreEqual(0, buf.numNotifications());
            buf.addNotification(notif);
            Assert::AreEqual(1, buf.numNotifications());
        }

        TEST_METHOD(NotificationBufferFixture_maxNotifs)
        {
            NotificationBuffer buf;
            Notification notif;

            for (int i = 0; i < 100; i++)
                buf.addNotification(notif);

            Assert::AreEqual(20, buf.numNotifications());
        }

    };
}

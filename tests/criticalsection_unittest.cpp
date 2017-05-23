#include "stdafx.h"
#include "CppUnitTest.h"

#include "critsec.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CriticalSectionFixture
{
    TEST_CLASS(CriticalSectionFixture)
    {
    public:

        TEST_METHOD(smokeTest)
        {
            WLock lock;

            bool e = false;
            try
            {
                CriticalSection cs(lock);
            }
            catch (...)
            {
                e = true;
            }
            Assert::AreNotEqual(e, true);
        }

        TEST_METHOD(recursive)
        {
            WLock lock;

            bool e = false;
            try
            {
                // デッドロックしない。
                CriticalSection cs(lock);
                {
                    CriticalSection cs(lock);
                }
            }
            catch (...)
            {
                e = true;
            }
            Assert::AreNotEqual(e, true);
        }

    };
}

#include "stdafx.h"
#include "CppUnitTest.h"

#include "defer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace DeferFixture
{
    TEST_CLASS(DeferFixture)
    {
    public:

        DeferFixture()
        {
            y = false;
        }

        void makeTrueAlways()
        {
            Defer makeTrue([=]() { y = true; });
            throw std::runtime_error("Oops!");
        }

        bool y;

        TEST_METHOD(DeferFixture_makeTrue)
        {
            bool x = false;
            {
                Defer makeTrue([&]() { x = true; });
                Assert::IsFalse(x);
            }
            Assert::IsTrue(x);
        }

        TEST_METHOD(DeferFixture_makeTrueAlways)
        {
            Assert::IsFalse(y);
            bool e = false;
            try
            {
                makeTrueAlways();
            }
            catch (std::runtime_error)
            {
                e = true;
            }
            Assert::AreEqual(e, true);
            Assert::IsTrue(y);
        }

    };
}

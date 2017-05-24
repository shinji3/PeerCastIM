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

        bool y;

        TEST_METHOD(makeTrue)
        {
            bool x = false;
            {
                Defer makeTrue([&]() { x = true; });
                Assert::IsFalse(x);
            }
            Assert::IsTrue(x);
        }

        TEST_METHOD(makeTrueAlways)
        {
            Assert::IsFalse(y);
            bool e = false;
            try
            {
                Defer makeTrue([=]() { y = true; });
                throw std::runtime_error("Oops!");
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

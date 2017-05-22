#include "stdafx.h"
#include "CppUnitTest.h"

#include "md5.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace md5Fixture
{
    TEST_CLASS(md5Fixture)
    {
    public:

        TEST_METHOD(test)
        {
            auto out = md5::hexdigest("hello");
            Assert::AreEqual("5d41402abc4b2a76b9719d911017c592", out.c_str());
        }

    };
}

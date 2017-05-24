#include "stdafx.h"
#include "CppUnitTest.h"

#include "id.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace IDStringFixture
{
    TEST_CLASS(IDStringFixture)
    {
    public:

    IDStringFixture()
        : idstr("abcde", 4),
          long_idstr("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 40)
    {
    }

    IDString idstr;
    IDString long_idstr;

    TEST_METHOD(TruncateToSpecifiedLength)
    {
        Assert::AreEqual("abcd", idstr.str());
    }

    TEST_METHOD(CanBeCastToConstCharPointer)
    {
        Assert::AreEqual("abcd", (const char*) idstr);
    }

    TEST_METHOD(TruncateTooLongString)
    {
        Assert::AreEqual(31, (int)strlen(long_idstr.str()));
    }

    };
}

#include "stdafx.h"
#include "CppUnitTest.h"

#include "regexp.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace RegexpFixture
{
    TEST_CLASS(RegexpFixture)
    {
    public:

        TEST_METHOD(RegexpFixture_nomatch)
        {
            Regexp pat("abc");
            Assert::AreEqual(0, pat.exec("hoge").size());
        }

        TEST_METHOD(RegexpFixture_match)
        {
            Regexp pat("abc");
            Assert::AreEqual(1, pat.exec("abc").size());
            Assert::AreEqual("abc", pat.exec("abc")[0]);
        }

        TEST_METHOD(RegexpFixture_captures)
        {
            Regexp pat("(.)(.)(.)");
            auto captures = pat.exec("abc");
            Assert::AreEqual(4, captures.size());
            Assert::AreEqual("abc", captures[0]);
            Assert::AreEqual("a", captures[1]);
            Assert::AreEqual("b", captures[2]);
            Assert::AreEqual("c", captures[3]);
        }

        TEST_METHOD(RegexpFixture_caretMatchesLineBeginning)
        {
            Regexp pat("^a");
            auto captures = pat.exec("b\na");
            Assert::AreEqual(1, captures.size());
            Assert::AreEqual("a", captures[0]);
        }

        TEST_METHOD(RegexpFixture_backslashCapitalADoesntMatchLineBeginning)
        {
            Regexp pat("\\Aa");
            auto captures = pat.exec("b\na");
            Assert::AreEqual(0, captures.size());
        }

    };
}

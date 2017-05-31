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
            Assert::AreEqual(0, (int)pat.exec("hoge").size());
        }

        TEST_METHOD(RegexpFixture_match)
        {
            Regexp pat("abc");
            Assert::AreEqual(1, (int)pat.exec("abc").size());
            Assert::AreEqual("abc", pat.exec("abc")[0].c_str());
        }

        TEST_METHOD(RegexpFixture_captures)
        {
            Regexp pat("(.)(.)(.)");
            auto captures = pat.exec("abc");
            Assert::AreEqual(4, (int)captures.size());
            Assert::AreEqual("abc", captures[0].c_str());
            Assert::AreEqual("a", captures[1].c_str());
            Assert::AreEqual("b", captures[2].c_str());
            Assert::AreEqual("c", captures[3].c_str());
        }

        TEST_METHOD(RegexpFixture_caretMatchesLineBeginning)
        {
            Regexp pat("^a");
            auto captures = pat.exec("b\na");
            Assert::AreEqual(1, (int)captures.size());
            Assert::AreEqual("a", captures[0].c_str());
        }

        TEST_METHOD(RegexpFixture_backslashCapitalADoesntMatchLineBeginning)
        {
            Regexp pat("\\Aa");
            auto captures = pat.exec("b\na");
            Assert::AreEqual(0, (int)captures.size());
        }

    };
}

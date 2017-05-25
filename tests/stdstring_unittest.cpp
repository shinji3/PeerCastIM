#include "stdafx.h"
#include "CppUnitTest.h"

#include <string>

using namespace std;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace stdStringFixture
{
    TEST_CLASS(stdStringFixture)
    {
    public:

        TEST_METHOD(stdStringFixture_size)
        {
            const char *p = "a\0b";
            Assert::AreEqual(3, (int)string(p, p + 3).size());

            // char* ‚©‚ç‰Šú‰»‚·‚éê‡‚Í \0 ‚ÅØ‚ê‚éB
            Assert::AreEqual(1, (int)string(p).size());
        }

        TEST_METHOD(stdStringFixture_equals)
        {
            Assert::IsTrue(string("a") != string("b"));
            Assert::IsTrue(string("a") != string("ab"));
            Assert::IsTrue(string("a\0b") == string("a"));
            Assert::IsTrue(string("abc") == string("abc\0"));
        }

        TEST_METHOD(stdStringFixture_substr)
        {
            string s = "0123456789";

            // 2‚Â–Ú‚Ìˆø”‚ÍˆÊ’u‚Å‚Í‚È‚­’·‚³B
            Assert::AreEqual("23", s.substr(2, 2).c_str());
        }

        TEST_METHOD(stdStringFixture_find)
        {
            string s = "0123456789";

            Assert::AreEqual(string::npos, s.find('A'));
            Assert::AreEqual(-1, (int)string::npos);
        }

        TEST_METHOD(stdStringFixture_initializerList)
        {
            string s = { 0, 1 };

            Assert::AreEqual(2, (int)s.size());
            Assert::AreEqual(0, (int)s[0]);
            Assert::AreEqual(1, (int)s[1]);
        }

    };
}

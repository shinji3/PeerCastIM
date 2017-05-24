#include "stdafx.h"
#include "CppUnitTest.h"

#include "http.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CookieFixture
{
    TEST_CLASS(CookieFixture)
    {
    public:

        Cookie c;

        TEST_METHOD(initialState)
        {
            Assert::AreEqual(0, (int)c.ip);
            Assert::AreEqual("", c.id);
            Assert::AreEqual(0, (int)c.time);
        }

        TEST_METHOD(set)
        {
            c.set("hoge", 0xffffffff);
            Assert::AreEqual(0xffffffff, c.ip);
            Assert::AreEqual("hoge", c.id);
        }

        TEST_METHOD(compare)
        {
            Cookie d, e, f, g;

            c.set("hoge", 0xffffffff);

            d.set("hoge", 0xffffffff);
            e.set("hoge", 0xfffffffe);
            f.set("fuga", 0xffffffff);
            g.set("fuga", 0xfffffffe);

            Assert::IsTrue(c.compare(d));
            Assert::IsTrue(d.compare(c));

            Assert::IsFalse(c.compare(e));
            Assert::IsFalse(e.compare(c));

            Assert::IsFalse(c.compare(f));
            Assert::IsFalse(f.compare(c));

            Assert::IsFalse(c.compare(g));
            Assert::IsFalse(g.compare(c));
        }

    };
}

#include "stdafx.h"
#include "CppUnitTest.h"

#include "cgi.h"

using namespace cgi;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace QueryFixture
{
    TEST_CLASS(QueryFixture)
    {
    public:

        QueryFixture()
            : query("a=1&b=2&b=3&c")
        {
        }

        Query query;

        TEST_METHOD(singleValuedKey)
        {
            Assert::AreEqual(true, query.hasKey("a"));
            Assert::AreEqual("1", query.get("a").c_str());
            Assert::AreEqual(1, (int)query.getAll("a").size());
            Assert::AreEqual("1", query.getAll("a")[0].c_str());
        }

        TEST_METHOD(multiValuedKey)
        {
            Assert::AreEqual(true, query.hasKey("b"));
            Assert::AreEqual("2", query.get("b").c_str());
            Assert::AreEqual(2, (int)query.getAll("b").size());
            Assert::AreEqual("2", query.getAll("b")[0].c_str());
            Assert::AreEqual("3", query.getAll("b")[1].c_str());
        }

        TEST_METHOD(keyWithNoValue)
        {
            Assert::AreEqual(true, query.hasKey("c"));
            Assert::AreEqual("", query.get("c").c_str());
            Assert::AreEqual(0, (int)query.getAll("c").size());
        }

        TEST_METHOD(nonexistentKey)
        {
            Assert::AreEqual(false, query.hasKey("d"));
            Assert::AreEqual("", query.get("d").c_str());
            Assert::AreEqual(0, (int)query.getAll("d").size());
            Assert::AreEqual(false, query.hasKey("d")); // still doesn't have the key
        }

    };
}

#include "stdafx.h"
#include "CppUnitTest.h"

#include <map>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace mapFixture
{
    TEST_CLASS(mapFixture)
    {
    public:

        map<string,string> dic;

        TEST_METHOD(bracketsDoesNotThrow)
        {
            Assert::AreEqual(0, dic.size());
            ASSERT_NO_THROW(dic["hoge"]);
            Assert::AreEqual("", dic["hoge"].c_str());
            Assert::AreEqual(1, dic.size());
        }

        TEST_METHOD(atThrows)
        {
            ASSERT_THROW(dic.at("hoge"), std::out_of_range);
        }

    };
}

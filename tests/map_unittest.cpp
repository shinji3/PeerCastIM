#include "stdafx.h"
#include "CppUnitTest.h"

#include <map>
#include <string>

using namespace std;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace mapFixture
{
    TEST_CLASS(mapFixture)
    {
    public:

        map<string,string> dic;

        TEST_METHOD(mapFixture_bracketsDoesNotThrow)
        {
            Assert::AreEqual(0, (int)dic.size());

            bool e = false;
            try
            {
                dic["hoge"];
            }
            catch (std::out_of_range)
            {
                e = true;
            }
            Assert::AreNotEqual(e, true);
            Assert::AreEqual("", dic["hoge"].c_str());
            Assert::AreEqual(1, (int)dic.size());
        }

        TEST_METHOD(mapFixture_atThrows)
        {
            bool e = false;
            try
            {
                dic.at("hoge");
            }
            catch (std::out_of_range)
            {
                e = true;
            }
            Assert::AreEqual(e, true);
        }

    };
}

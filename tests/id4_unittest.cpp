#include "stdafx.h"
#include "CppUnitTest.h"

#include "id.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ID4Fixture
{
    TEST_CLASS(ID4Fixture)
    {
    public:

        ID4Fixture()
            : pid(new ID4()),
            abcd("abcd"),
            abcd2('abcd')
        {
        }

        ~ID4Fixture()
        {
            delete pid;
        }

        ID4 id;
        ID4* pid;
        ID4 abcd;
        ID4 abcd2;

        TEST_METHOD(ID4Fixture_initializedToZero)
        {
            Assert::AreEqual(0, id.getValue());
            Assert::AreEqual(0, pid->getValue());
        }

        TEST_METHOD(ID4Fixture_equalsAndNotEqual)
        {
            Assert::AreEqual((int)id, (int)*pid);
            Assert::AreNotEqual((int)id, (int)abcd);
        }

        // 複数文字の文字定数はリトル・エンディアンの環境で文字の順番が反転す
        // ることが期待される。
        TEST_METHOD(ID4Fixture_getValue)
        {
            uint16_t n = 0xabcd;
            uint8_t *p = (uint8_t*)&n;

            if (*p == 0xab) {
                // ビッグ
                Assert::AreEqual(ID4('abcd').getValue(), ID4("abcd").getValue());
            }
            else if (*p == 0xcd) {
                // リトル
                Assert::AreNotEqual(ID4('abcd').getValue(), ID4("abcd").getValue());
            }
            else {
                // 何も信じられない。
                Assert::IsTrue(false);
            }
        }


    };
}

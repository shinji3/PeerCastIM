#include "stdafx.h"
#include "CppUnitTest.h"

#include "common.h"
#include "mocksys.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// GnuID::generate() が sys->rnd() に依存している。

namespace GnuIDFixture
{
    TEST_CLASS(GnuIDFixture)
    {
    public:

        GnuID id;
        char buf[80]; // 最低 33 バイト必要。

        TEST_METHOD(GnuIDFixture_initialState)
        {
            // GnuID はコンストラクタで初期化されないので、不定値が入っている。
            // オール0のIDが欲しければ明示的に clear メソッドでクリアする。
            // id.toStr(buf);
            // EXPECT_TRUE(id.isSet());
            // EXPECT_STRNE("00000000000000000000000000000000", buf);

            Assert::IsFalse(id.isSet());
            id.toStr(buf);
            Assert::AreEqual("00000000000000000000000000000000", buf);
        }

        TEST_METHOD(GnuIDFixture_generate)
        {
            id.toStr(buf);
            Assert::IsFalse(id.isSet());
            Assert::AreEqual("00000000000000000000000000000000", buf);

            GnuID id2;
            id2.clear();
            Assert::IsTrue(id.isSame(id2));

            id.generate();
            Assert::IsFalse(id.isSame(id2));
        }

        TEST_METHOD(GnuIDFixture_getFlags)
        {
            Assert::AreEqual(0, (int)id.getFlags());

            id.generate(0xff);
            Assert::AreEqual(0xff, (int)id.getFlags());
        }

        TEST_METHOD(GnuIDFixture_clear)
        {
            id.generate();
            Assert::IsTrue(id.isSet());

            id.clear();
            Assert::IsFalse(id.isSet());
        }

        TEST_METHOD(GnuIDFixture_encode)
        {
            Assert::IsFalse(id.isSet());
            id.encode(NULL, "A", NULL, 0);
            Assert::IsTrue(id.isSet());

            GnuID id2;
            id2.encode(NULL, "A", NULL, 0);
            Assert::IsTrue(id.isSame(id2));

            Assert::AreEqual("41004100410041004100410041004100", id.str().c_str());
        }

        TEST_METHOD(GnuIDFixture_encode2)
        {
            id.encode(NULL, "A", "B", 0);
            Assert::AreEqual("03000300030003000300030003000300", id.str().c_str());
        }

        TEST_METHOD(GnuIDFixture_encode3)
        {
            id.encode(NULL, "A", "B", 1);
            Assert::AreEqual("02010201020102010201020102010201", id.str().c_str());
        }

        TEST_METHOD(GnuIDFixture_encode4)
        {
            id.encode(NULL, "AB", NULL, 0);
            Assert::AreEqual("41420041420041420041420041420041", id.str().c_str());
        }

        TEST_METHOD(GnuIDFixture_encode_differentSeeds)
        {
            Assert::IsFalse(id.isSet());
            id.encode(NULL, "A", NULL, 0);

            GnuID id2;
            id2.fromStr("00000000000000000000000000000001");
            Assert::IsTrue(id.isSet());
            id2.encode(NULL, "A", NULL, 0);
            Assert::IsFalse(id.isSame(id2));

            Assert::AreEqual("41004100410041004100410041004100", id.str().c_str());
            Assert::AreEqual("41004100410041004100410041004101", id2.str().c_str());
        }

        TEST_METHOD(GnuIDFixture_encode_differentSalts)
        {
            id.encode(NULL, "A", NULL, 0);

            GnuID id2;
            id2.encode(NULL, "B", NULL, 0);
            Assert::IsFalse(id.isSame(id2));

            Assert::AreEqual("41004100410041004100410041004100", id.str().c_str());
            Assert::AreEqual("42004200420042004200420042004200", id2.str().c_str());
        }

        TEST_METHOD(GnuIDFixture_encode_prefixSharingSalts)
        {
            id.encode(NULL, "ナガイナマエ", NULL, 0);

            GnuID id2;
            id2.encode(NULL, "ナガイナマエ(立て直し)", NULL, 0);
            Assert::IsFalse(id.isSame(id2));
        }

    };
}

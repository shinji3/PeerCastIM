#include "stdafx.h"
#include "CppUnitTest.h"

#include "matroska.h"

using namespace matroska;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace VIntFixture
{
    TEST_CLASS(VIntFixture)
    {
    public:

        TEST_METHOD(numLeadingZeroes)
        {
            uint8_t n = 1;

            Assert::AreEqual(8, VInt::numLeadingZeroes(0));
            Assert::AreEqual(7, VInt::numLeadingZeroes(1));
            Assert::AreEqual(6, VInt::numLeadingZeroes(1 << 1));
            Assert::AreEqual(5, VInt::numLeadingZeroes(1 << 2));
            Assert::AreEqual(4, VInt::numLeadingZeroes(1 << 3));
            Assert::AreEqual(3, VInt::numLeadingZeroes(1 << 4));
            Assert::AreEqual(2, VInt::numLeadingZeroes(1 << 5));
            Assert::AreEqual(1, VInt::numLeadingZeroes(1 << 6));
            Assert::AreEqual(0, VInt::numLeadingZeroes(1 << 7));
        }

        TEST_METHOD(uint64)
        {
            Assert::AreEqual(1, (int)VInt({ 0x81 }).uint());

            // 2^56 - 1
            Assert::AreEqual(((uint64_t)1 << 56) - 1,
                VInt({ 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }).uint());
            // 2^14 - 1
            Assert::AreEqual((1 << 14) - 1,
                (int)VInt({ 0x7f, 0xff }).uint());
        }

        TEST_METHOD(constructor)
        {
            bool e = false;
            try
            {
                VInt(matroska::byte_string({ 0xff }));
            }
            catch (std::runtime_error)
            {
                e = true;
            }
            Assert::AreEqual(e, true);
        }

        TEST_METHOD(idToName)
        {
            Assert::AreEqual("EBML", VInt({ 0x1A,0x45,0xDF,0xA3 }).toName().c_str());
        }

    };
}

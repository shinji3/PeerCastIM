#include "stdafx.h"
#include "CppUnitTest.h"

#include "mkv.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MKVStreamFixture
{
    TEST_CLASS(MKVStreamFixture)
    {
    public:

        TEST_METHOD(MKVStreamFixture_unpackUnsignedInt)
        {
            bool e = false;
            try
            {
                MKVStream::unpackUnsignedInt("");
            }
            catch (std::runtime_error)
            {
                e = true;
            }
            Assert::AreEqual(e, true);
            Assert::AreEqual(1, (int)MKVStream::unpackUnsignedInt("\x01"));
            Assert::AreEqual(258, (int)MKVStream::unpackUnsignedInt("\x01\x02"));
        }

    };
}

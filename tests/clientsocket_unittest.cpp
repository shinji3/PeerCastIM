#include "stdafx.h"
#include "CppUnitTest.h"

#include "win32/wsocket.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ClientSocketFixture
{
    TEST_CLASS(ClientSocketFixture)
    {
    public:

        TEST_METHOD(getIP)
        {
            Assert::AreEqual((127<<24 | 1), ClientSocket::getIP("localhost"));
            Assert::AreEqual((127<<24 | 1), ClientSocket::getIP("127.0.0.1"));
        }

    };
}

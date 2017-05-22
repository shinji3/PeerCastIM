#include "stdafx.h"
#include "CppUnitTest.h"

#include "win32/wsocket.h"
#include "win32/wsys.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ClientSocketFixture
{
    TEST_CLASS(ClientSocketFixture)
    {
    public:

        TEST_METHOD(getIP)
        {
            printf("%d\n", (127 << 24 | 1));
            printf("%d\n", (int)ClientSocket::getIP("localhost"));
            Assert::AreEqual((127<<24 | 1), (int)ClientSocket::getIP("localhost"));
            Assert::AreEqual((127<<24 | 1), (int)ClientSocket::getIP("127.0.0.1"));
        }

    };
}

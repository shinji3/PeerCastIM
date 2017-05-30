#include "stdafx.h"
#include "CppUnitTest.h"

#include "win32\wsocket.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ClientSocketFixture
{
    TEST_CLASS(ClientSocketFixture)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            peercastApp = new MockPeercastApplication();
            peercastInst = new MockPeercastInstance();
            peercastInst->init();
        }

        ClientSocketFixture()
        {
            WSAClientSocket::init();
        }

        ~ClientSocketFixture()
        {
            WSACleanup();
        }

        TEST_METHOD(ClientSocketFixture_getIP)
        {
            Assert::AreEqual((127<<24 | 1), (int)ClientSocket::getIP("localhost"));
            Assert::AreEqual((127<<24 | 1), (int)ClientSocket::getIP("127.0.0.1"));
        }

    };
}

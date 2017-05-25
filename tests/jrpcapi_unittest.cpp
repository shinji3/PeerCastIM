#include "stdafx.h"
#include "CppUnitTest.h"

#include "jrpc.h"

using json = nlohmann::json;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace JrpcApiFixture
{
    TEST_CLASS(JrpcApiFixture)
    {
    public:

        JrpcApi api;

        TEST_METHOD(JrpcApiFixture_methodListIsInitialized)
        {
            Assert::AreNotEqual(0, (int)api.m_methods.size());
        }

        TEST_METHOD(JrpcApiFixture_toPositionalArguments)
        {
            json named_params = {
                {"a", 1},
                {"b", 2}
            };
            json names = { "a", "b" };

            json result = api.toPositionalArguments(named_params,
                names);

            Assert::IsTrue(json::array({ 1, 2 }) == result);
        }

        TEST_METHOD(JrpcApiFixture_getNewVersions)
        {
            json result = api.getNewVersions(json::array());

            Assert::IsTrue(json::array() == result);
        }

        TEST_METHOD(JrpcApiFixture_getNotificationMessages)
        {
            json result = api.getNotificationMessages(json::array());

            Assert::IsTrue(json::array() == result);
        }

        TEST_METHOD(JrpcApiFixture_getStatus)
        {
            json result = api.getStatus(json::array());

            std::string localIP = Host(ClientSocket::getIP(NULL), 7144).IPtoStr();

            json expected = {
                { "globalDirectEndPoint", { "127.0.0.1", 7144 } },
                { "globalRelayEndPoint", { "127.0.0.1", 7144 } },
                { "isFirewalled", nullptr },
                { "localDirectEndPoint", { localIP, 7144 } },
                { "localRelayEndPoint", { localIP, 7144 } },
                { "uptime", 0 },
            };

            Assert::IsTrue(expected == result);
        }

        TEST_METHOD(JrpcApiFixture_getChannelRelayTree)
        {
            bool e = false;
            try
            {
                api.getChannelRelayTree({ "hoge" });
            }
            catch (JrpcApi::application_error)
            {
                e = true;
            }
            Assert::AreEqual(e, true);
        }

    };
}

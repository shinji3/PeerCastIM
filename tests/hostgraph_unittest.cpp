#include "stdafx.h"
#include "CppUnitTest.h"

#include "jrpc.h"
#include "defer.h"

using json = nlohmann::json;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace HostGraphFixture
{
    TEST_CLASS(HostGraphFixture)
    {
    public:

        TEST_METHOD(HostGraphFixture_constructorNullChannel)
        {
            bool e = false;
            try
            {
                JrpcApi::HostGraph(nullptr, nullptr);
            }
            catch (std::invalid_argument)
            {
                e = true;
            }
            Assert::AreEqual(e, true);
        }

        TEST_METHOD(HostGraphFixture_simplestCase)
        {
            auto ch = new Channel();
            auto hitList = new ChanHitList();
            Defer reclaim([=]() { delete ch; delete hitList; });

            JrpcApi::HostGraph graph(ch, hitList);

            Assert::AreEqual("[{\"address\":\"127.0.0.1\",\"children\":[],\"isControlFull\":false,\"isDirectFull\":true,\"isFirewalled\":true,\"isReceiving\":false,\"isRelayFull\":false,\"isTracker\":false,\"localDirects\":0,\"localRelays\":0,\"port\":0,\"sessionId\":\"00151515151515151515151515151515\",\"version\":1218}]", ((json)graph.getRelayTree()).dump().c_str());
        }

        TEST_METHOD(HostGraphFixture_withHitList)
        {
            auto ch = new Channel();
            auto hitList = new ChanHitList();
            Defer reclaim([=]() { delete ch; delete hitList; });
            ChanHit hit;

            hit.init();
            hit.rhost[0].fromStrIP("8.8.8.8", 7144);
            hit.rhost[1].fromStrIP("192.168.0.1", 7144);

            hitList->addHit(hit);

            JrpcApi::HostGraph graph(ch, hitList);

            Assert::AreEqual("[{\"address\":\"127.0.0.1\",\"children\":[],\"isControlFull\":false,\"isDirectFull\":true,\"isFirewalled\":true,\"isReceiving\":false,\"isRelayFull\":false,\"isTracker\":false,\"localDirects\":0,\"localRelays\":0,\"port\":0,\"sessionId\":\"00151515151515151515151515151515\",\"version\":1218},{\"address\":\"8.8.8.8\",\"children\":[],\"isControlFull\":false,\"isDirectFull\":true,\"isFirewalled\":false,\"isReceiving\":true,\"isRelayFull\":false,\"isTracker\":false,\"localDirects\":0,\"localRelays\":0,\"port\":7144,\"sessionId\":\"00000000000000000000000000000000\",\"version\":0}]", ((json)graph.getRelayTree()).dump().c_str());
        }

    };
}

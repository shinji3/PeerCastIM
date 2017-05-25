#include "stdafx.h"
#include "CppUnitTest.h"

#include "channel.h"
#include "version2.h"
#include "md5.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ChanMgrFixture
{
    TEST_CLASS(ChanMgrFixture)
    {
    public:

        ChanMgrFixture()
        {
            x = new ChanMgr();
        }

        ~ChanMgrFixture()
        {
            delete x;
        }
        ChanMgr* x;

        TEST_METHOD(ChanMgrFixture_constants)
        {
            Assert::AreEqual(8, (int)ChanMgr::MAX_IDLE_CHANNELS);
            Assert::AreEqual(8192, (int)ChanMgr::MAX_METAINT);
        }

        TEST_METHOD(ChanMgrFixture_initialState)
        {
            GnuID id;
            ChanInfo info;

            id.clear();

            Assert::IsNull(x->channel);
            Assert::IsNull(x->hitlist);
            Assert::AreEqual(PCP_BROADCAST_FLAGS, (char)x->broadcastID.getFlags());

            Assert::IsTrue(id.isSame(x->searchInfo.id));
            Assert::IsTrue(id.isSame(x->searchInfo.bcID));
            // ...

            // Assert::AreEqual(0, x->numFinds); // ‰Šú‰»‚³‚ê‚È‚¢B
            Assert::AreEqual(String(), x->broadcastMsg.c_str());
            Assert::AreEqual(10, (int)x->broadcastMsgInterval);
            //Assert::AreEqual(0, x->lastHit); // ‰Šú‰»‚³‚ê‚È‚¢B
            Assert::AreEqual(0, (int)x->lastQuery);
            Assert::AreEqual(0, (int)x->maxUptime);
            // Assert::AreEqual(true, x->searchActive); // ‰Šú‰»‚³‚ê‚È‚¢B
            Assert::AreEqual(600, (int)x->deadHitAge);
            Assert::AreEqual(8192, x->icyMetaInterval);
            Assert::AreEqual(0, x->maxRelaysPerChannel);
            Assert::AreEqual(1, x->minBroadcastTTL);
            Assert::AreEqual(7, x->maxBroadcastTTL);
            Assert::AreEqual(60, x->pushTimeout);
            Assert::AreEqual(5, x->pushTries);
            Assert::AreEqual(8, x->maxPushHops);
            Assert::AreEqual(0, (int)x->autoQuery);
            Assert::AreEqual(10, (int)x->prefetchTime);
            Assert::AreEqual(0, (int)x->lastYPConnect);
            // Assert::AreEqual(0, x->lastYPConnect2);
            Assert::AreEqual(0, (int)x->icyIndex);
            Assert::AreEqual(120, (int)x->hostUpdateInterval);
            Assert::AreEqual(5, (int)x->bufferTime);
            Assert::IsTrue(id.isSame(x->currFindAndPlayChannel));
        }

        TEST_METHOD(ChanMgrFixture_createChannel)
        {
            ChanInfo info;
            Channel *c;

            Assert::IsNull(x->channel);

            c = x->createChannel(info, NULL);

            Assert::IsTrue(c);
            Assert::AreEqual(c->channel_id, x->channel->channel_id);

            x->deleteChannel(c);
        }

        TEST_METHOD(ChanMgrFixture_authSecret)
        {
            Assert::AreEqual("00151515151515151515151515151515:01234567890123456789012345678901", x->authSecret("01234567890123456789012345678901").c_str());
        }

        TEST_METHOD(ChanMgrFixture_authToken)
        {
            Assert::AreEqual("44d5299e57ad9274fee7960a9fa60bfd", x->authToken("01234567890123456789012345678901").c_str());
            Assert::AreEqual("44d5299e57ad9274fee7960a9fa60bfd", md5::hexdigest("00151515151515151515151515151515:01234567890123456789012345678901").c_str());
        }
    };
}

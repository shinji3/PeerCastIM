#include "stdafx.h"
#include "CppUnitTest.h"

#include "channel.h"
#include "servmgr.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ChanHitListFixture
{
    TEST_CLASS(ChanHitListFixture)
    {
    public:

        ChanHitListFixture()
        {
            hitlist = new ChanHitList();

            host.fromStrIP("209.209.209.209", 7144);
            global.fromStrIP("210.210.210.210", 7144);
            local.fromStrIP("192.168.0.2", 7144);

            hit.init();
            hit.host = host;
            hit.rhost[0] = global;
            hit.rhost[1] = local;
        }

        void TearDown()
        {
            delete hitlist;
        }

        ChanHit hit;
        ChanHitList* hitlist;
        Host host;
        Host global;
        Host local;

        TEST_METHOD(ChanHitListFixture_initialState)
        {
            Assert::AreEqual(false, hitlist->used);

            Assert::AreEqual(NULL, hitlist->hit);

            Assert::AreEqual(0, hitlist->lastHitTime);
        }

        TEST_METHOD(ChanHitListFixture_contactTrackers)
        {
            // 使われてない。
            Assert::AreEqual(0, hitlist->contactTrackers(false, 0, 0, 0));
        }

        template <typename T>
        static int listCount(T* list)
        {
            int count = 0;

            while (list != NULL)
            {
                count++;
                list = list->next;
            }
            return count;
        }

        TEST_METHOD(ChanHitListFixture_addHit)
        {
            hitlist->addHit(hit);

            Assert::AreEqual(1, listCount(hitlist));

            // 同じホストを追加しても増えない。
            hitlist->addHit(hit);

            Assert::AreEqual(1, listCount(hitlist));
        }

        TEST_METHOD(ChanHitListFixture_isUsed)
        {
            Assert::AreEqual(false, hitlist->isUsed());
        }

        TEST_METHOD(ChanHitListFixture_createXML)
        {
            hitlist->addHit(hit);

            {
                XML::Node* root = hitlist->createXML();
                MemoryStream mem(1024);

                root->write(mem, 0);
                mem.buf[mem.pos] = '\0';

                Assert::AreEqual("<hits hosts=\"1\" listeners=\"0\" relays=\"0\" firewalled=\"0\" closest=\"0\" furthest=\"0\" newest=\"0\">\n<host ip=\"209.209.209.209:7144\" hops=\"0\" listeners=\"0\" relays=\"0\" uptime=\"0\" push=\"0\" relay=\"1\" direct=\"0\" cin=\"1\" stable=\"0\" version=\"0\" update=\"0\" tracker=\"0\"/>\n</hits>\n", mem.buf);
                delete root;
            }

            {
                XML::Node* root = hitlist->createXML(false);
                MemoryStream mem(1024);

                root->write(mem, 0);
                mem.buf[mem.pos] = '\0';

                Assert::AreEqual("<hits hosts=\"1\" listeners=\"0\" relays=\"0\" firewalled=\"0\" closest=\"0\" furthest=\"0\" newest=\"0\"/>\n", mem.buf);
                delete root;
            }

        }

        TEST_METHOD(ChanHitListFixture_deleteHit)
        {
            Assert::AreEqual(NULL, hitlist->hit);

            hitlist->addHit(hit);

            Assert::AreEqual(1, listCount(hitlist->hit));
            // Assert::AreNotEqual(NULL, hitlist->hit); // なんでコンパイルできない？

            Assert::AreEqual(NULL, hitlist->deleteHit(hitlist->hit));
            Assert::AreEqual(0, listCount(hitlist->hit));
        }

        TEST_METHOD(ChanHitListFixture_getTotalListeners)
        {
            Assert::AreEqual(0, hitlist->getTotalListeners());

            hit.numListeners = 10;
            hitlist->addHit(hit);

            Assert::AreEqual(10, hitlist->getTotalListeners());
        }

        TEST_METHOD(ChanHitListFixture_getTotalRelays)
        {
            Assert::AreEqual(0, hitlist->getTotalListeners());

            hit.numRelays = 10;
            hitlist->addHit(hit);

            Assert::AreEqual(10, hitlist->getTotalRelays());
        }

        TEST_METHOD(ChanHitListFixture_getTotalFirewalled)
        {
            Assert::AreEqual(0, hitlist->getTotalFirewalled());

            hit.firewalled = true;
            hitlist->addHit(hit);

            Assert::AreEqual(1, hitlist->getTotalFirewalled());
        }

        // TEST_METHOD(ChanHitListFixture_getSeq)
        // {
        //     unsigned int seq = hitlist->getSeq();

        //     Assert::AreEqual(seq + 1, hitlist->getSeq());
        // }

        TEST_METHOD(ChanHitListFixture_forEachHit)
        {
            ChanHit h1, h2, h3;

            hitlist->used = true;

            h1.rhost[0].fromStrIP("0.0.0.1", 7144);
            h2.rhost[0].fromStrIP("0.0.0.2", 7144);
            h3.rhost[0].fromStrIP("0.0.0.3", 7144);

            hitlist->addHit(h1);
            hitlist->addHit(h2);
            hitlist->addHit(h3);

            int count = 0;
            hitlist->forEachHit([&](ChanHit*) { count++; });
            Assert::AreEqual(3, count);
        }

    };
}

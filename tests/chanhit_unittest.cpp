#include "stdafx.h"
#include "CppUnitTest.h"

#include "atom.h"
#include "channel.h"

#include "mockpeercast.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ChanHitFixture
{
    TEST_CLASS(ChanHitFixture)
    {
    public:

        ChanHitFixture()
            : hit(new ChanHit())
        {
            peercastApp = new MockPeercastApplication();
            peercastInst = new MockPeercastInstance();
            peercastInst->init();

            hit->init();
        }

        ~ChanHitFixture()
        {
            delete hit;
        }

        ChanHit* hit;


        // 初期状態を確かめるだけ。
        TEST_METHOD(ChanHitFixture_initialState)
        {
            Assert::AreEqual(0, (int)hit->host.ip);
            Assert::AreEqual(0, (int)hit->host.port);

            Assert::AreEqual(0, (int)hit->rhost[0].ip);
            Assert::AreEqual(0, (int)hit->rhost[0].port);

            Assert::AreEqual(0, (int)hit->rhost[1].ip);
            Assert::AreEqual(0, (int)hit->rhost[1].port);

            Assert::AreEqual(0, (int)hit->numListeners);
            Assert::AreEqual(0, (int)hit->numRelays);
            Assert::AreEqual(0, (int)hit->numHops);
            // Assert::AreEqual(0, hit->clap_pp);
            Assert::AreEqual(0, (int)hit->time);
            Assert::AreEqual(0, (int)hit->upTime);
            Assert::AreEqual(0, (int)hit->lastContact);
            //Assert::AreEqual(0, hit->hitID);

            Assert::IsFalse(hit->sessionID.isSet());
            Assert::IsFalse(hit->chanID.isSet());

            Assert::AreEqual(0, (int)hit->version);
            Assert::AreEqual(0, (int)hit->versionVP);

            Assert::AreEqual(false, hit->firewalled);
            Assert::AreEqual(false, hit->stable);
            Assert::AreEqual(false, hit->tracker);
            Assert::AreEqual(true, hit->recv);
            Assert::AreEqual(false, hit->yp);
            Assert::AreEqual(false, hit->dead);
            Assert::AreEqual(false, hit->direct);
            Assert::AreEqual(true, hit->relay);
            Assert::AreEqual(true, hit->cin);
            // Assert::AreEqual(false, hit->relayfull);
            // Assert::AreEqual(false, hit->chfull);
            // Assert::AreEqual(false, hit->ratefull);

            Assert::AreEqual(NULL, (int)hit->next);

            // Assert::AreEqual(0, hit->status);
            // Assert::AreEqual(0, hit->servent_id);
            Assert::AreEqual(0, (int)hit->oldestPos);
            Assert::AreEqual(0, (int)hit->newestPos);

            Assert::AreEqual(0, (int)hit->uphost.ip);
            Assert::AreEqual(0, (int)hit->uphost.port);

            Assert::AreEqual(0, (int)hit->uphostHops);

            Assert::AreEqual(' ', hit->versionExPrefix[0]);
            Assert::AreEqual(' ', hit->versionExPrefix[1]);

            Assert::AreEqual(0, (int)hit->versionExNumber);
            // Assert::AreEqual(0, hit->lastSendSeq);
        }

        TEST_METHOD(ChanHitFixture_pickNearestIP)
        {
            hit->rhost[0].fromStrIP("210.210.210.210", 8145); // global
            hit->rhost[1].fromStrIP("192.168.0.2", 8145); // local

            Host host;

            {
                // pick local
                host.fromStrIP("192.168.0.1", 7144);

                hit->pickNearestIP(host);

                char ip[22];
                hit->host.toStr(ip);

                Assert::AreEqual("192.168.0.2:8145", ip);
            }

            {
                // pick global
                host.fromStrIP("209.209.209.209", 7144);

                hit->pickNearestIP(host);

                char ip[22];
                hit->host.toStr(ip);

                Assert::AreEqual("210.210.210.210:8145", ip);
            }
        }

        // TEST_METHOD(ChanHitFixture_initLocal_pp_Stealth)
        // {
        //     hit->numListeners = 100;

        //     hit->initLocal_pp(true, 10);

        //     Assert::AreEqual(0, hit->numListeners);
        //     Assert::AreEqual(10, hit->clap_pp);
        // }

        // TEST_METHOD(ChanHitFixture_initLocal_pp_NonStealth)
        // {
        //     hit->numListeners = 100;

        //     hit->initLocal_pp(false, 10);

        //     Assert::AreEqual(1, hit->numListeners);
        //     Assert::AreEqual(10, hit->clap_pp);
        // }

        TEST_METHOD(ChanHitFixture_createXML)
        {
            XML::Node* node = hit->createXML();

            MemoryStream stream(1024);
            node->write(stream, 0);
            int length = stream.pos;

            char buf[1024];
            stream.rewind();
            stream.read(buf, length);
            buf[length] = '\0';

            Assert::AreEqual("<host ip=\"0.0.0.0:0\" hops=\"0\" listeners=\"0\" relays=\"0\" uptime=\"0\" push=\"0\" relay=\"1\" direct=\"0\" cin=\"1\" stable=\"0\" version=\"0\" update=\"0\" tracker=\"0\"/>\n", buf);

            delete node;
        }

        static char* asString(MemoryStream& mem, char* buf)
        {
            int length = mem.pos;
            mem.rewind();
            mem.read(buf, length);
            buf[length] = '\0';
            return buf;
        }

        TEST_METHOD(ChanHitFixture_writeVariableNonExistent)
        {
            MemoryStream mem(1024);
            bool res;

            res = hit->writeVariable(mem, "foo");
            Assert::AreEqual(false, res);
        }

        TEST_METHOD(ChanHitFixture_writeVariable)
        {
#define TEST_VARIABLE(name, value)                          \
    do {                                                    \
        MemoryStream mem(1024);                             \
        char buf[1025];                                     \
        Assert::AreEqual(true, hit->writeVariable(mem, name));     \
        Assert::AreEqual(value, (name, asString(mem, buf)));    \
    } while (0)

            TEST_VARIABLE("rhost0", "<font color=green>0.0.0.0:0</font>");
            TEST_VARIABLE("rhost1", "0.0.0.0:0");
            TEST_VARIABLE("numHops", "0");
            TEST_VARIABLE("numListeners", "0");
            TEST_VARIABLE("numRelays", "0");
            TEST_VARIABLE("uptime", "-");
            TEST_VARIABLE("update", "-");
            TEST_VARIABLE("isFirewalled", "0");
            TEST_VARIABLE("version", "0"); // original
            
            // TEST_VARIABLE("version", "-");
            // TEST_VARIABLE("agent", "0");
            // TEST_VARIABLE("check", "<a href=\"#\" onclick=\"checkip('0.0.0.0')\">_</a>");
            // TEST_VARIABLE("uphost", "0.0.0.0:0");
            // TEST_VARIABLE("uphostHops", "0");
            // TEST_VARIABLE("canRelay", "1");
        }

        // 条件変数: chanID.isSet()     +0 +24
        //           versionExNumber==0 +0 +20
        //           uphost.ip==0       +0 +36
        // 2^3 = 8 通りのパスがある。
        // 常に送信されるサイズは 8 + 24 + 12+10 + 12+10 + 12*5 + 9 + 12*2 = 169 バイト
        TEST_METHOD(ChanHitFixture_writeAtom000)
        {
            MemoryStream mem(1024);
            AtomStream writer(mem);

            GnuID chid;
            chid.clear();
            hit->versionExNumber = 0;
            hit->uphost.ip = 0;
            Assert::IsFalse(chid.isSet());
            hit->writeAtoms(writer, chid);
            Assert::AreEqual(169, mem.pos);
            // Assert::AreEqual(157, mem.pos); // オリジナルの0.1218では157バイトになる。
        }

        TEST_METHOD(ChanHitFixture_writeAtom001)
        {
            MemoryStream mem(1024);
            AtomStream writer(mem);
            GnuID chid;
            chid.clear();
            hit->versionExNumber = 0;
            hit->uphost.ip = 1;
            hit->writeAtoms(writer, chid);
            Assert::AreEqual(169 + 36, mem.pos);
        }

        TEST_METHOD(ChanHitFixture_writeAtom010)
        {
            MemoryStream mem(1024);
            AtomStream writer(mem);
            GnuID chid;
            chid.clear();
            hit->versionExNumber = 1;
            hit->uphost.ip = 0;
            hit->writeAtoms(writer, chid);
            Assert::AreEqual(169 + 20, mem.pos);
        }

        TEST_METHOD(ChanHitFixture_writeAtom100)
        {
            MemoryStream mem(1024);
            AtomStream writer(mem);
            GnuID chid;
            chid.fromStr("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");

            hit->versionExNumber = 0;
            hit->uphost.ip = 0;
            hit->writeAtoms(writer, chid);
            Assert::AreEqual(169 + 24, mem.pos);
        }

        TEST_METHOD(ChanHitFixture_initLocal)
        {
            Channel channel;

            int numl = 1;
            int numr = 2;
            int nums = 3; // 使われていない。
            int uptm = 4;
            bool connected = true;
            bool isFull = true;
            unsigned int bitrate = 5; // 使われていない。
            Channel* ch = &channel;
            unsigned int oldp = 6;
            unsigned int newp = 7;

            hit->initLocal(numl, numr, nums, uptm, connected,
                /* isFull, bitrate, ch, */
                oldp, newp);

            Assert::AreEqual(1, (int)hit->numListeners);
            Assert::AreEqual(2, (int)hit->numRelays);

            Assert::AreEqual(4, (int)hit->upTime);
            Assert::AreEqual(true, hit->recv);
            // Assert::AreEqual(true, hit->chfull);

            Assert::AreEqual(6, (int)hit->oldestPos);
            Assert::AreEqual(7, (int)hit->newestPos);
        }

        TEST_METHOD(ChanHitFixture_str)
        {
            Assert::AreEqual("0.0.0.0", hit->str().c_str());
        }

        TEST_METHOD(ChanHitFixture_strOriginal)
        {
            hit->version = 1218;
            Assert::AreEqual("0.0.0.0 (1218)", hit->str().c_str());
        }

        TEST_METHOD(ChanHitFixture_strVP)
        {
            hit->version = 1218;
            hit->versionVP = 26;
            Assert::AreEqual("0.0.0.0 (VP26)", hit->str().c_str());
        }

        TEST_METHOD(ChanHitFixture_strIM)
        {
            hit->version = 1218;
            hit->versionVP = 27;
            hit->versionExPrefix[0] = 'I';
            hit->versionExPrefix[1] = 'M';
            hit->versionExNumber = 51;
            Assert::AreEqual("0.0.0.0 (IM51)", hit->str().c_str());
        }

        TEST_METHOD(ChanHitFixture_versionString)
        {
            Assert::AreEqual("", hit->versionString().c_str());
        }

        TEST_METHOD(ChanHitFixture_versionStringOriginal)
        {
            hit->version = 1218;
            Assert::AreEqual("1218", hit->versionString().c_str());
        }

        TEST_METHOD(ChanHitFixture_versionStringVP)
        {
            hit->version = 1218;
            hit->versionVP = 26;
            Assert::AreEqual("VP26", hit->versionString().c_str());
        }

        TEST_METHOD(ChanHitFixture_versionStringIM)
        {
            hit->version = 1218;
            hit->versionVP = 27;
            hit->versionExPrefix[0] = 'I';
            hit->versionExPrefix[1] = 'M';
            hit->versionExNumber = 51;
            Assert::AreEqual("IM51", hit->versionString().c_str());
        }

    };
}

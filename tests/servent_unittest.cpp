#include "stdafx.h"
#include "CppUnitTest.h"

#include "str.h"

#include "servent.h"
#include "sstream.h"

#include "defer.h"
#include "servmgr.h"

#include "mockclientsocket.h"

using namespace cgi;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ServentFixture
{
    TEST_CLASS(ServentFixture)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            peercastApp = new MockPeercastApplication();
            peercastInst = new MockPeercastInstance();
            peercastInst->init();
        }

        ServentFixture()
            : s(0) {}
        Servent s;

        TEST_METHOD(ServentFixture_initialState)
        {
            Assert::AreEqual((int)Servent::T_NONE, (int)s.type);
            Assert::AreEqual((int)Servent::S_NONE, (int)s.status);

            // static const char   *statusMsgs[], *typeMsgs[];

            // GnuStream           gnuStream;
            // GnuPacket           pack;

            Assert::AreEqual(0, (int)s.lastConnect);
            Assert::AreEqual(0, (int)s.lastPing);
            Assert::AreEqual(0, (int)s.lastPacket);
            Assert::AreEqual("", s.agent.cstr());

            Assert::AreEqual(0, s.seenIDs.numUsed());
            Assert::AreEqual("00000000000000000000000000000000", s.networkID.str().c_str());
            Assert::AreEqual(0, s.serventIndex);

            Assert::AreEqual("00000000000000000000000000000000", s.remoteID.str().c_str());
            Assert::AreEqual("00000000000000000000000000000000", s.chanID.str().c_str());
            Assert::AreEqual("00000000000000000000000000000000", s.givID.str().c_str());

            Assert::AreEqual(false, (bool)s.thread.active);

            Assert::AreEqual("", s.loginPassword.cstr());
            Assert::AreEqual("", s.loginMount.cstr());

            Assert::AreEqual(false, s.priorityConnect);
            Assert::AreEqual(false, s.addMetadata);

            Assert::AreEqual(0, s.nsSwitchNum);

            Assert::AreEqual((int)Servent::ALLOW_ALL, (int)s.allow);

            Assert::IsNull(s.sock);
            Assert::IsNull(s.pushSock);

            // WLock               lock;

            Assert::AreEqual(true, s.sendHeader);
            // Assert::AreEqual(0, s.syncPos); // 不定
            // Assert::AreEqual(0, s.streamPos);  // 不定
            Assert::AreEqual(0, s.servPort);

            Assert::AreEqual((int)ChanInfo::SP_UNKNOWN, (int)s.outputProtocol);

            // GnuPacketBuffer     outPacketsNorm, outPacketsPri;

            Assert::AreEqual(false, s.flowControl);

            Assert::IsNull(s.next);

            Assert::IsNull(s.pcpStream);

            Assert::AreEqual(0, (int)s.cookie.time);
            Assert::AreEqual("", s.cookie.id);
            Assert::AreEqual(0, (int)s.cookie.ip);

        }

        TEST_METHOD(ServentFixture_handshakeHTTP)
        {
            MockClientSocket* mock;
            Defer reclaim([&]() { delete mock; });

            s.sock = mock = new MockClientSocket();

            HTTP http(*mock);
            http.initRequest("GET / HTTP/1.0");
            mock->incoming.str("\r\n");

            s.handshakeHTTP(http, true);

            Assert::AreEqual("HTTP/1.0 302 Found\r\nLocation: /html/en/index.html\r\n\r\n",
                mock->outgoing.str().c_str());
        }

        TEST_METHOD(ServentFixture_handshakeIncomingGetRoot)
        {
            MockClientSocket* mock;
            Defer reclaim([&]() { delete mock; });

            s.sock = mock = new MockClientSocket();
            mock->incoming.str("GET / HTTP/1.0\r\n\r\n");

            s.handshakeIncoming();

            Assert::AreEqual("HTTP/1.0 302 Found\r\nLocation: /html/en/index.html\r\n\r\n",
                mock->outgoing.str().c_str());
        }

        // servMgr->password が設定されていない時に ShoutCast クライアントから
        // の放送要求だとして通してしまうが、良いのか？
        TEST_METHOD(ServentFixture_handshakeIncomingBadRequest)
        {
            MockClientSocket* mock;

            s.sock = mock = new MockClientSocket();
            mock->incoming.str("\r\n");

            bool e = false;
            try
            {
                s.handshakeIncoming();
            }
            catch (StreamException)
            {
                e = true;
            }
            Assert::AreEqual(e, true);

            delete mock;
        }

        TEST_METHOD(ServentFixture_handshakeIncomingHTMLRoot)
        {
            MockClientSocket* mock;

            s.sock = mock = new MockClientSocket();
            mock->incoming.str("GET /html/en/index.html HTTP/1.0\r\n\r\n");

            s.handshakeIncoming();

            std::string output = mock->outgoing.str();

            // ファイルが無いのに OK はおかしくないか…
            Assert::IsTrue(str::contains(output, "200 OK"));
            Assert::IsTrue(str::contains(output, "Server: "));
            Assert::IsTrue(str::contains(output, "Date: "));
            Assert::IsTrue(str::contains(output, "Unable to open file"));

            delete mock;
        }

        TEST_METHOD(ServentFixture_handshakeIncomingJRPCGetUnauthorized)
        {
            MockClientSocket* mock;

            s.sock = mock = new MockClientSocket();
            mock->incoming.str("GET /api/1 HTTP/1.0\r\n\r\n");

            bool e = false;
            try
            {
                s.handshakeIncoming();
            }
            catch (std::runtime_error)
            {
                e = true;
            }
            Assert::AreNotEqual(e, true);

            std::string output = mock->outgoing.str();

            Assert::AreEqual("HTTP/1.0 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"PeerCast Admin\"\r\n\r\n", output.c_str());

            delete mock;
        }

        TEST_METHOD(ServentFixture_handshakeIncomingJRPCGetAuthorized)
        {
            MockClientSocket* mock;

            strcpy_s(servMgr->password, _countof(servMgr->password), "Passw0rd");

            // --------------------------------------------
            s.sock = mock = new MockClientSocket();
            mock->incoming.str("GET /api/1 HTTP/1.0\r\n"
                "\r\n");

            s.handshakeIncoming();

            Assert::IsTrue(str::contains(mock->outgoing.str(), "401 Unauthorized"));

            delete mock;

            // --------------------------------------------

            s.sock = mock = new MockClientSocket();
            mock->incoming.str("GET /api/1 HTTP/1.0\r\n"
                "Authorization: BASIC OlBhc3N3MHJk\r\n" // ruby -rbase64 -e 'p Base64.strict_encode64 ":Passw0rd"'
                "\r\n");

            s.handshakeIncoming();

            Assert::IsTrue(str::contains(mock->outgoing.str(), "200 OK"));
            Assert::IsTrue(str::contains(mock->outgoing.str(), "jsonrpc"));

            delete mock;
        }

        //                  |<---------------------- 77 characters long  -------------------------------->|
#define LONG_STRING "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI"

// 8470 bytes
#define LONG_LONG_STRING LONG_STRING \
    LONG_STRING \
    LONG_STRING \
    LONG_STRING \
    LONG_STRING \
    LONG_STRING \
    LONG_STRING \
    LONG_STRING \
    LONG_STRING \
    LONG_STRING \
    LONG_STRING

// 8191 バイト以上のリクエストに対してエラーを返す。
        TEST_METHOD(ServentFixture_handshakeIncomingLongURI)
        {
            Assert::AreEqual(8470, (int)strlen(LONG_LONG_STRING));

            MockClientSocket* mock;

            s.sock = mock = new MockClientSocket();
            mock->incoming.str("GET /" LONG_LONG_STRING " HTTP/1.0\r\n"
                "\r\n");

            bool e = false;
            try
            {
                s.handshakeIncoming();
            }
            catch (HTTPException)
            {
                e = true;
            }
            Assert::AreEqual(e, true);

            delete mock;
        }

        TEST_METHOD(ServentFixture_createChannelInfoNullCase)
        {
            Query query("");
            auto info = s.createChannelInfo(GnuID(), String(), query, "");

            Assert::AreEqual((int)ChanInfo::T_UNKNOWN, (int)info.contentType);
            Assert::AreEqual("", info.name.cstr());
            Assert::AreEqual("", info.genre.cstr());
            Assert::AreEqual("", info.desc.cstr());
            Assert::AreEqual("", info.url.cstr());
            Assert::AreEqual(0, info.bitrate);
            Assert::AreEqual("", info.comment);
        }

        TEST_METHOD(ServentFixture_createChannelInfoComment)
        {
            Query query("");
            auto info = s.createChannelInfo(GnuID(), "俺たちみんなトドだぜ (・ω・｀з)3", query, "");

            Assert::AreEqual("俺たちみんなトドだぜ (・ω・｀з)3", info.comment);
        }

        TEST_METHOD(ServentFixture_createChannelInfoCommentOverride)
        {
            Query query("comment=スレなし");
            auto info = s.createChannelInfo(GnuID(), "俺たちみんなトドだぜ (・ω・｀з)3", query, "");

            Assert::AreEqual("スレなし", info.comment);
        }

        TEST_METHOD(ServentFixture_createChannelInfoTypicalCase)
        {
            Query query("name=予定地&genre=テスト&desc=てすと&url=http://example.com&comment=スレなし&bitrate=400&type=mkv");
            auto info = s.createChannelInfo(GnuID(), String(), query, "");

            Assert::AreEqual((int)ChanInfo::T_MKV, (int)info.contentType);
            Assert::AreEqual("予定地", info.name.cstr());
            Assert::AreEqual("テスト", info.genre.cstr());
            Assert::AreEqual("てすと", info.desc.cstr());
            Assert::AreEqual("http://example.com", info.url.cstr());
            Assert::AreEqual(400, info.bitrate);
            Assert::AreEqual("スレなし", info.comment);
        }

        TEST_METHOD(ServentFixture_createChannelInfoNonnumericBitrate)
        {
            Query query("bitrate=BITRATE");
            ChanInfo info;

            bool e = false;
            try
            {
                info = s.createChannelInfo(GnuID(), String(), query, "");
            }
            catch (...)
            {
                e = true;
            }
            Assert::AreNotEqual(e, true);


            Assert::AreEqual(0, info.bitrate);
        }

        TEST_METHOD(ServentFixture_hasValidAuthToken)
        {
            Assert::IsTrue(s.hasValidAuthToken("01234567890123456789012345678901.flv?auth=44d5299e57ad9274fee7960a9fa60bfd"));
            Assert::IsFalse(s.hasValidAuthToken("01234567890123456789012345678901.flv?auth=00000000000000000000000000000000"));
            Assert::IsFalse(s.hasValidAuthToken("01234567890123456789012345678901.flv?"));
            Assert::IsFalse(s.hasValidAuthToken("01234567890123456789012345678901.flv"));
            Assert::IsFalse(s.hasValidAuthToken(""));
            Assert::IsFalse(s.hasValidAuthToken("ほげほげ.flv?auth=44d5299e57ad9274fee7960a9fa60bfd"));
            Assert::IsFalse(s.hasValidAuthToken("ほげほげほげほげほげほげほげほげほげほげほげほげほげほげほげほげほげほげほげほげほげほげ.flv?auth=44d5299e57ad9274fee7960a9fa60bfd"));
            Assert::IsFalse(s.hasValidAuthToken("?auth=44d5299e57ad9274fee7960a9fa60bfd"));
        }

    };
}

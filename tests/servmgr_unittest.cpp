#include "stdafx.h"
#include "CppUnitTest.h"

#include "servmgr.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ServMgrFixture
{
    TEST_CLASS(ServMgrFixture)
    {
    public:

        ServMgr m;

        TEST_METHOD(ServMgrFixture_initialState)
        {
            // Servent             *servents;
            Assert::AreEqual(nullptr, m.servents);
            // WLock               lock;
            // ServHost            hostCache[MAX_HOSTCACHE];
            // char                password[64];
            Assert::AreEqual("", m.password);
            // bool                allowGnutella;
            Assert::IsFalse(m.allowGnutella);
            // unsigned int        maxBitrateOut, maxControl, maxRelays, maxDirect;
            Assert::AreEqual(0, m.maxBitrateOut);
            Assert::AreEqual(3, m.maxControl);
            Assert::AreEqual(2, m.maxRelays);
            Assert::AreEqual(0, m.maxDirect);
            // unsigned int        minGnuIncoming, maxGnuIncoming;
            Assert::AreEqual(10, m.minGnuIncoming);
            Assert::AreEqual(20, m.maxGnuIncoming);
            // unsigned int        maxServIn;
            Assert::AreEqual(50, m.maxServIn);
            // bool                isDisabled;
            Assert::IsFalse(m.isDisabled);
            // bool                isRoot;
            Assert::IsFalse(m.isRoot);
            // int                 totalStreams;
            Assert::AreEqual(0, m.totalStreams);
            // Host                serverHost;
            Assert::AreEqual("127.0.0.1:7144", m.serverHost.str().c_str());
            // String              rootHost;
            Assert::AreEqual("yp.peercast.org", m.rootHost.cstr());
            // char                downloadURL[128];
            Assert::AreEqual("", m.downloadURL);
            // String              rootMsg;
            Assert::AreEqual("", m.rootMsg.cstr());
            // String              forceIP;
            Assert::AreEqual("", m.forceIP.cstr());
            // char                connectHost[128];
            Assert::AreEqual("connect1.peercast.org", m.connectHost);
            // GnuID               networkID;
            Assert::AreEqual("00000000000000000000000000000000", m.networkID.str().c_str());
            // unsigned int        firewallTimeout;
            Assert::AreEqual(30, m.firewallTimeout);
            // int                 showLog;
            Assert::AreEqual(0, m.showLog);
            // int                 shutdownTimer;
            Assert::AreEqual(0, m.shutdownTimer);
            // bool                pauseLog;
            Assert::IsFalse(m.pauseLog);
            // bool                forceNormal;
            Assert::IsFalse(m.forceNormal);
            // bool                useFlowControl;
            Assert::IsTrue(m.useFlowControl);
            // unsigned int        lastIncoming;
            Assert::AreEqual(0, m.lastIncoming);
            // bool                restartServer;
            Assert::IsFalse(m.restartServer);
            // bool                allowDirect;
            Assert::IsTrue(m.allowDirect);
            // bool                autoConnect, autoServe, forceLookup;
            Assert::IsTrue(m.autoConnect);
            Assert::IsTrue(m.autoServe);
            Assert::IsTrue(m.forceLookup);
            // int                 queryTTL;
            Assert::AreEqual(7, m.queryTTL);
            // unsigned int        allowServer1, allowServer2;
            Assert::AreEqual(Servent::ALLOW_ALL, m.allowServer1);
            Assert::AreEqual(Servent::ALLOW_BROADCAST, m.allowServer2);
            // unsigned int        startTime;
            Assert::AreEqual(0, m.startTime);
            // unsigned int        tryoutDelay;
            Assert::AreEqual(10, m.tryoutDelay);
            // unsigned int        refreshHTML;
            Assert::AreEqual(5, m.refreshHTML);
            // unsigned int        relayBroadcast;
            //Assert::AreEqual(0, m.relayBroadcast); // 不定
            // unsigned int        notifyMask;
            Assert::AreEqual(0xffff, m.notifyMask);
            // BCID                *validBCID;
            Assert::AreEqual(nullptr, m.validBCID);
            // GnuID               sessionID;
            Assert::AreEqual("00151515151515151515151515151515", m.sessionID.str().c_str());
            // ServFilter          filters[MAX_FILTERS];
            Assert::AreEqual(0xffffffff, m.filters[0].host.ip);
            Assert::AreEqual(ServFilter::F_NETWORK | ServFilter::F_DIRECT, m.filters[0].flags);
            // int                 numFilters;
            Assert::AreEqual(1, m.numFilters);
            // CookieList          cookieList;
            // AUTH_TYPE           authType;
            Assert::AreEqual(ServMgr::AUTH_COOKIE, m.authType);
            // char                htmlPath[128];
            Assert::AreEqual("html/en", m.htmlPath);
            // int                 serventNum;
            Assert::AreEqual(0, m.serventNum);
            // String              chanLog;
            Assert::AreEqual("", m.chanLog.cstr());
            // ChannelDirectory    channelDirectory;
            // bool                publicDirectoryEnabled;
            Assert::IsFalse(m.publicDirectoryEnabled);
            // FW_STATE            firewalled;
            Assert::AreEqual(ServMgr::FW_UNKNOWN, m.firewalled);
        }

    };
}

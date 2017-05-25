#include "stdafx.h"
#include "CppUnitTest.h"

#include "common.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace HostTest
{
    TEST_CLASS(HostTest)
    {
    public:

        TEST_METHOD(HostTest_loopbackIP) {
            Host host;

            host.fromStrIP("127.0.0.1", 0);
            Assert::IsTrue(host.loopbackIP());

            // 127 で始まるクラスAのネットワーク全部がループバックとして機能す
            // るが、loopbackIP は 127.0.0.1 以外には FALSE を返す。
            host.fromStrIP("127.99.99.99", 0);
            Assert::IsFalse(host.loopbackIP());
        }

        TEST_METHOD(HostTest_isMemberOf) {
            Host host, pattern;

            host.fromStrIP("192.168.0.1", -1);
            pattern.fromStrIP("192.168.0.1", -1);
            Assert::IsTrue(host.isMemberOf(pattern));

            pattern.fromStrIP("192.168.0.2", -1);
            Assert::IsFalse(host.isMemberOf(pattern));

            pattern.fromStrIP("192.168.0.255", -1);
            Assert::IsTrue(host.isMemberOf(pattern));

            pattern.fromStrIP("192.168.255.255", -1);
            Assert::IsTrue(host.isMemberOf(pattern));

            pattern.fromStrIP("192.168.255.1", -1);
            Assert::IsTrue(host.isMemberOf(pattern));

            pattern.fromStrIP("0.0.0.0", -1);
            Assert::IsFalse(host.isMemberOf(pattern));

            host.fromStrIP("0.0.0.0", -1);
            pattern.fromStrIP("0.0.0.0", -1);
            Assert::IsFalse(host.isMemberOf(pattern));
        }

        TEST_METHOD(HostTest_str)
        {
            Host host;
            Assert::AreEqual("0.0.0.0:0", host.str().c_str());
            Assert::AreEqual("0.0.0.0:0", host.str(true).c_str());
            Assert::AreEqual("0.0.0.0", host.str(false).c_str());
        }

        TEST_METHOD(HostTest_strUlimit)
        {
            Host host;
            host.fromStrIP("255.255.255.255", 65535);
            Assert::AreEqual("255.255.255.255:65535", host.str().c_str());
            Assert::AreEqual("255.255.255.255:65535", host.str(true).c_str());
            Assert::AreEqual("255.255.255.255", host.str(false).c_str());
        }

    };
}

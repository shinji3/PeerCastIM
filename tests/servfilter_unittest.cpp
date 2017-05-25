#include "stdafx.h"
#include "CppUnitTest.h"

#include "sstream.h"
#include "servmgr.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ServFilterFixture
{
    TEST_CLASS(ServFilterFixture)
    {
    public:

        ServFilter filter;
        StringStream mem;

        TEST_METHOD(ServFilterFixture_initialState)
        {
            Assert::AreEqual(0, (int)filter.flags);
            Assert::AreEqual(0, (int)filter.host.ip);
        }

        TEST_METHOD(ServFilterFixture_writeVariable)
        {
            mem.str("");
            filter.writeVariable(mem, "network");
            Assert::AreEqual("0", mem.str().c_str());

            mem.str("");
            filter.writeVariable(mem, "private");
            Assert::AreEqual("0", mem.str().c_str());

            mem.str("");
            filter.writeVariable(mem, "direct");
            Assert::AreEqual("0", mem.str().c_str());

            mem.str("");
            filter.writeVariable(mem, "banned");
            Assert::AreEqual("0", mem.str().c_str());

            mem.str("");
            filter.writeVariable(mem, "ip");
            Assert::AreEqual("0.0.0.0", mem.str().c_str());
        }

        TEST_METHOD(ServFilterFixture_writeVariableNetwork)
        {
            filter.flags |= ServFilter::F_NETWORK;
            filter.writeVariable(mem, "network");
            Assert::AreEqual("1", mem.str().c_str());
        }

        TEST_METHOD(ServFilterFixture_writeVariablePrivate)
        {
            filter.flags |= ServFilter::F_PRIVATE;
            filter.writeVariable(mem, "private");
            Assert::AreEqual("1", mem.str().c_str());
        }

        TEST_METHOD(ServFilterFixture_writeVariableDirect)
        {
            filter.flags |= ServFilter::F_DIRECT;
            filter.writeVariable(mem, "direct");
            Assert::AreEqual("1", mem.str().c_str());
        }

        TEST_METHOD(ServFilterFixture_writeVariableBanned)
        {
            filter.flags |= ServFilter::F_BAN;
            filter.writeVariable(mem, "banned");
            Assert::AreEqual("1", mem.str().c_str());
        }

        TEST_METHOD(ServFilterFixture_writeVariableIP)
        {
            filter.host.ip = (127 << 24) | 1;
            filter.writeVariable(mem, "ip");
            Assert::AreEqual("127.0.0.1", mem.str().c_str());
        }

    };
}

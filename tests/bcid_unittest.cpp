#include "stdafx.h"
#include "CppUnitTest.h"

#include "servmgr.h"
#include "sstream.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BCIDFixture
{
    TEST_CLASS(BCIDFixture)
    {
    public:

        StringStream mem;
        BCID bcid;

        TEST_METHOD(BCIDFixture_initialState)
        {
            Assert::AreEqual("00000000000000000000000000000000", bcid.id.str().c_str());
            Assert::AreEqual("", bcid.name.cstr());
            Assert::AreEqual("", bcid.email.cstr());
            Assert::AreEqual("", bcid.url.cstr());
            Assert::AreEqual(true, bcid.valid);
            Assert::IsNull(bcid.next);
        }

        TEST_METHOD(BCIDFixture_writeVariable)
        {
            bcid.writeVariable(mem, "id");
            Assert::AreEqual("00000000000000000000000000000000", mem.str().c_str());
        }

        TEST_METHOD(BCIDFixture_writeVariable_name)
        {
            bool written = bcid.writeVariable(mem, "name");
            Assert::IsTrue(written);
            Assert::AreEqual("", mem.str().c_str());
        }

        TEST_METHOD(BCIDFixture_writeVariable_email)
        {
            bool written = bcid.writeVariable(mem, "email");
            Assert::IsTrue(written);
            Assert::AreEqual("", mem.str().c_str());
        }

        TEST_METHOD(BCIDFixture_writeVariable_url)
        {
            bool written = bcid.writeVariable(mem, "url");
            Assert::IsTrue(written);
            Assert::AreEqual("", mem.str().c_str());
        }

        TEST_METHOD(BCIDFixture_writeVariable_valid)
        {
            bcid.writeVariable(mem, "valid");
            Assert::AreEqual("Yes", mem.str().c_str());
        }
    };
}

#include "stdafx.h"
#include "CppUnitTest.h"

#include "http.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace HTTPHeadersFixture
{
    TEST_CLASS(HTTPHeadersFixture)
    {
    public:

        HTTPHeaders headers;

        TEST_METHOD(HTTPHeadersFixture_set)
        {
            headers.set("Content-Length", "123");
            Assert::AreEqual("123", headers.m_headers["CONTENT-LENGTH"].c_str());
        }

        TEST_METHOD(HTTPHeadersFixture_get)
        {
            headers.set("Content-Length", "123");
            Assert::AreEqual("123", headers.get("Content-Length").c_str());
            Assert::AreEqual("123", headers.get("CONTENT-LENGTH").c_str());
            Assert::AreEqual("123", headers.get("content-length").c_str());
        }

        TEST_METHOD(HTTPHeadersFixture_copyConstruct)
        {
            headers.set("a", "b");
            HTTPHeaders copy(headers);
            Assert::AreEqual("b", copy.get("A").c_str());
        }

        TEST_METHOD(HTTPHeadersFixture_assignment)
        {
            headers.set("a", "b");
            HTTPHeaders copy;
            Assert::AreEqual("", copy.get("A").c_str());
            copy = headers;
            Assert::AreEqual("b", copy.get("A").c_str());
        }

    };
}

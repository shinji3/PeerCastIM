#include "stdafx.h"
#include "CppUnitTest.h"

#include "sstream.h"
#include "http.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace HTTPFixture
{
    TEST_CLASS(HTTPFixture)
    {
    public:

        HTTPFixture()
            : http(mem)
        {
        }
        StringStream mem;
        HTTP http;

        TEST_METHOD(HTTPFixture_readResponse)
        {
            mem.str("HTTP/1.0 200 OK\r\n");
            int statusCode = http.readResponse();

            Assert::AreEqual(200, statusCode);
            // 副作用として cmdLine がちょんぎれる。
            Assert::AreEqual("HTTP/1.0 200", http.cmdLine);
            Assert::IsTrue(mem.eof());
        }

        TEST_METHOD(HTTPFixture_checkResponseReturnsTrue)
        {
            mem.str("HTTP/1.0 200 OK\r\n");

            Assert::IsTrue(http.checkResponse(200));
            Assert::IsTrue(mem.eof());
        }


        TEST_METHOD(HTTPFixture_checkResponseThrows)
        {
            mem.str("HTTP/1.0 404 Not Found\r\n");

            ASSERT_THROW(http.checkResponse(200), StreamException);
            Assert::IsTrue(mem.eof());
        }

        TEST_METHOD(HTTPFixture_readRequest)
        {
            mem.str("GET /index.html HTTP/1.0\r\n");
            http.readRequest();
            Assert::AreEqual("GET /index.html HTTP/1.0", http.cmdLine);
            Assert::IsTrue(mem.eof());
        }

        TEST_METHOD(HTTPFixture_nextHeader)
        {
            mem.str("GET /index.html HTTP/1.0\r\n"
                "Host: localhost\r\n"
                "Connection: close\r\n"
                "\r\n");

            http.readRequest();
            Assert::AreEqual(0, http.headers.size());
            Assert::AreEqual(NULL, http.arg);

            Assert::IsTrue(http.nextHeader());
            Assert::AreEqual(1, http.headers.size());
            Assert::AreEqual("localhost", http.arg);

            Assert::IsTrue(http.nextHeader());
            Assert::AreEqual(2, http.headers.size());
            Assert::AreEqual("close", http.arg);

            Assert::IsFalse(http.nextHeader());
            Assert::AreEqual(2, http.headers.size());
            Assert::AreEqual(NULL, http.arg);

            Assert::AreEqual("localhost", http.headers.get("Host").c_str());
            Assert::AreEqual("close", http.headers.get("Connection").c_str());
        }

        TEST_METHOD(HTTPFixture_isHeader)
        {
            mem.str("GET /index.html HTTP/1.0\r\n"
                "Host: localhost\r\n"
                "Connection: close\r\n"
                "\r\n");

            http.readRequest();

            Assert::IsTrue(http.nextHeader());
            Assert::IsTrue(http.isHeader("Host"));
            Assert::IsTrue(http.isHeader("host")); // case-insensitive
            Assert::IsTrue(http.isHeader("localhost")); // 値の部分にもマッチしちゃう
            Assert::IsTrue(http.isHeader("h")); // 実は前方一致
            Assert::IsFalse(http.isHeader("")); // でも空文字列はダメ

            Assert::IsTrue(http.nextHeader());
            Assert::IsTrue(http.isHeader("Connection"));

            Assert::IsFalse(http.nextHeader());
        }

        TEST_METHOD(HTTPFixture_isRequest)
        {
            mem.str("GET /index.html HTTP/1.0\r\n");

            http.readRequest();

            Assert::IsTrue(http.isRequest("GET"));
            Assert::IsFalse(http.isRequest("POST"));
            Assert::IsFalse(http.isRequest("get")); // case-sensitive
            Assert::IsTrue(http.isRequest("G"));
            Assert::IsFalse(http.isRequest("ET"));
            Assert::IsTrue(http.isRequest("GET "));
            Assert::IsTrue(http.isRequest(""));
        }

        TEST_METHOD(HTTPFixture_getArgStr)
        {
            Assert::AreEqual(NULL, http.arg);
            Assert::AreEqual(NULL, http.getArgStr());

            http.arg = (char*)"hoge";
            Assert::AreEqual(http.arg, http.getArgStr());
        }

        TEST_METHOD(HTTPFixture_getArgInt)
        {
            http.arg = NULL;

            Assert::AreEqual(NULL, http.arg);
            Assert::AreEqual(0, http.getArgInt());

            http.arg = (char*)"123";
            Assert::AreEqual(123, http.getArgInt());

            http.arg = (char*)"";
            Assert::AreEqual(0, http.getArgInt());

            http.arg = (char*)"hoge";
            Assert::AreEqual(0, http.getArgInt());
        }

        TEST_METHOD(HTTPFixture_getAuthUserPass)
        {
            mem.str("Authorization: BASIC OlBhc3N3MHJk\r\n");

            Assert::IsTrue(http.nextHeader());
            Assert::IsTrue(http.isHeader("Authorization"));
            char user[100] = "dead", pass[100] = "beef";
            http.getAuthUserPass(user, pass, 100, 100);

            Assert::AreEqual("", user);
            Assert::AreEqual("Passw0rd", pass);
        }

        TEST_METHOD(HTTPFixture_getAuthUserPass2)
        {
            mem.str("HOGEHOGEHOGE: hogehogehoge\r\n");

            Assert::IsTrue(http.nextHeader());
            char user[100] = "dead", pass[100] = "beef";
            http.getAuthUserPass(user, pass, 100, 100);

            Assert::AreEqual("dead", user);
            Assert::AreEqual("beef", pass);
        }

        TEST_METHOD(HTTPFixture_initRequest)
        {
            http.initRequest("GET /index.html HTTP/1.0\r\n");
            // readRequest と違って、改行コードは削除されない。
            Assert::AreEqual("GET /index.html HTTP/1.0\r\n", http.cmdLine);
        }

        TEST_METHOD(HTTPFixture_reset)
        {
            mem.str("GET /index.html HTTP/1.0\r\n"
                "Host: localhost\r\n"
                "Connection: close\r\n"
                "\r\n");

            http.readRequest();
            http.nextHeader();

            Assert::AreEqual("Host: localhost", http.cmdLine);
            Assert::AreEqual("localhost", http.arg);
            Assert::AreEqual(1, http.headers.size());

            http.reset();

            Assert::AreEqual("", http.cmdLine);
            Assert::AreEqual(NULL, http.arg);
            Assert::AreEqual(0, http.headers.size());
        }

        TEST_METHOD(HTTPFixture_initialState)
        {
            Assert::AreEqual("", http.cmdLine);
            Assert::AreEqual(NULL, http.arg);
            Assert::AreEqual(0, http.headers.size());
        }

    };
}

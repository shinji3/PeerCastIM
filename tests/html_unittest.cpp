#include "stdafx.h"
#include "CppUnitTest.h"

#include "html.h"
#include "sstream.h"
#include "version2.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace HTMLFixture
{
    TEST_CLASS(HTMLFixture)
    {
    public:

        HTMLFixture()
            : html("Untitled", mem)
        {
        }

        StringStream mem;
        HTML html;

        TEST_METHOD(HTMLFixture_initialState)
        {
            Assert::AreEqual("Untitled", html.title.cstr());
            Assert::AreEqual(0, html.tagLevel);
            Assert::AreEqual(0, html.refresh);
        }

        TEST_METHOD(HTMLFixture_writeOK)
        {
            html.writeOK("application/octet-stream");
            // Assert::AreEqual("HTTP/1.0 200 OK\r\nServer: "
            //              PCX_AGENT "\r\n"
            //              "Connection: close\r\n"
            //              "Content-Type: application/octet-stream\r\n"
            //              "\r\n", mem.str().c_str());
            Assert::AreEqual("HTTP/1.0 200 OK\r\nServer: "
                PCX_AGENT "\r\n"
                "Connection: close\r\n"
                "Content-Type: application/octet-stream\r\n"
                "Date: Thu, 01 Jan 1970 00:00:00 GMT\r\n"
                "\r\n", mem.str().c_str());
        }

        TEST_METHOD(HTMLFixture_writeOKwithHeaders)
        {
            // html.writeOK("application/octet-stream", { {"Date", "hoge"} });
        }

        TEST_METHOD(HTMLFixture_locateTo)
        {
            html.locateTo("/index.html");
            Assert::AreEqual("HTTP/1.0 302 Found\r\nLocation: /index.html\r\n\r\n",
                mem.str().c_str());
        }

        TEST_METHOD(HTMLFixture_addHead)
        {
            html.addHead();
            Assert::AreEqual("<head><title>Untitled</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"></meta></head>", mem.str().c_str());
        }


        // タイトルはフォーマット文字列として解釈されてはいけない。
        TEST_METHOD(HTMLFixture_addHead2)
        {
            StringStream mem2;
            HTML html2("%s%s%s%s%s%s", mem2);

            html2.addHead();
            Assert::AreEqual("<head><title>%s%s%s%s%s%s</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"></meta></head>", mem2.str().c_str());
        }

    };
}

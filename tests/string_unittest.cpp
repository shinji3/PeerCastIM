#include "stdafx.h"
#include "CppUnitTest.h"

#include "sys.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace StringTest
{
    TEST_CLASS(StringTest)
    {
    public:

        TEST_METHOD(StringTest_isEmptyWorks) {
            String str;
            Assert::IsTrue(str.isEmpty());
        }

        TEST_METHOD(StringTest_setFromStopwatchWorks) {
            String str;
            str.setFromStopwatch(60 * 60 * 24);
            Assert::AreEqual("1 day, 0 hour", str);

            str.setFromStopwatch(0);
            Assert::AreEqual("-", str);
        }

        TEST_METHOD(StringTest_eqWorks) {
            String s1("abc");
            String s2("abc");
            String s3("ABC");
            String s4("xyz");

            Assert::AreEqual(s1, s2.cstr());
            Assert::AreNotEqual(s1, s3.cstr());
            Assert::AreNotEqual(s1, s4.cstr());
        }

        TEST_METHOD(StringTest_containsWorks) {
            String abc("abc");
            String ABC("ABC");
            String xyz("xyz");
            String a("a");

            Assert::IsTrue(abc.contains(ABC));
            Assert::IsFalse(abc.contains(xyz));
            Assert::IsFalse(ABC.contains(xyz));

            Assert::IsTrue(abc.contains(a));
            Assert::IsFalse(a.contains(abc));
        }

        TEST_METHOD(StringTest_appendChar) {
            String buf;
            for (int i = 0; i < 500; i++) {
                buf.append('A');
            }

            // バッファーは 256 バイトあるので、NUL の分を考慮しても 255 文字まで入る。
            Assert::AreEqual(255, (int)strlen(buf.data));
        }

        TEST_METHOD(StringTest_appendString) {
            String s = "a";

            s.append("bc");

            Assert::AreEqual("abc", s.cstr());
        }

        TEST_METHOD(StringTest_prependWorks) {
            String buf = " world!";
            buf.prepend("Hello");
            Assert::AreEqual("Hello world!", buf);

            String buf2 = " world!AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
            buf2.prepend("Hello");
            Assert::AreEqual("Hello", buf2);
        }

        TEST_METHOD(StringTest_isWhitespaceWorks) {
            Assert::IsTrue(String::isWhitespace(' '));
            Assert::IsTrue(String::isWhitespace('\t'));
            Assert::IsFalse(String::isWhitespace('A'));
            Assert::IsFalse(String::isWhitespace('-'));
            Assert::IsFalse(String::isWhitespace('\r'));
            Assert::IsFalse(String::isWhitespace('\n'));
        }

        TEST_METHOD(StringTest_ASCII2HTMLWorks) {
            String str;

            str.ASCII2HTML("AAA");
            Assert::AreEqual("AAA", str);

            // 英数字以外は16進の文字実体参照としてエスケープされる。
            str.ASCII2HTML("   ");
            Assert::AreEqual("&#x20;&#x20;&#x20;", str);

            str.ASCII2HTML("&\"<>");
            Assert::AreEqual("&#x26;&#x22;&#x3C;&#x3E;", str);
        }

        // TEST_METHOD(StringTest_ASCII2HTMLWorks) {
        //     String str;

        //     str.ASCII2HTML("AAA");
        //     Assert::AreEqual("AAA", str);

        //     str.ASCII2HTML("   ");
        //     Assert::AreEqual("&#x20;&#x20;&#x20;", str);

        //     String str2("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"); // 255
        //     str.ASCII2HTML(str2);
        //     Assert::AreEqual(246, strlen(str.data));

        //     str.ASCII2HTML("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA ");
        //     Assert::AreEqual(251, strlen(str.data));
        // }

        TEST_METHOD(StringTest_HTML2ASCIIWorks) {
            String str;

            str.HTML2ASCII("&#x21;");
            Assert::AreEqual("!", str);

            str.HTML2ASCII("A");
            Assert::AreEqual("A", str);

            str.HTML2ASCII("&amp;");
            Assert::AreEqual("&amp;", str);
        }

        TEST_METHOD(StringTest_setFromStopwatch)
        {
            String s;

            Assert::AreEqual("1 day, 0 hour", (s.setFromStopwatch(86400), s).cstr());
            Assert::AreEqual("1 hour, 0 min", (s.setFromStopwatch(3600), s).cstr());
            Assert::AreEqual("1 min, 0 sec", (s.setFromStopwatch(60), s).cstr());
            Assert::AreEqual("1 sec", (s.setFromStopwatch(1), s).cstr());
            Assert::AreEqual("-", (s.setFromStopwatch(0), s).cstr());
        }

        TEST_METHOD(StringTest_assignment)
        {
            String s, t;

            s = "hoge";
            t = s;
            Assert::AreEqual("hoge", t.cstr());
        }

        TEST_METHOD(StringTest_sjisToUtf8)
        {
            String tmp = "4\x93\xFA\x96\xDA"; // "4日目" in Shit_JIS
            tmp.convertTo(String::T_UNICODESAFE);
            Assert::AreEqual("4\xE6\x97\xA5\xE7\x9B\xAE", tmp.cstr());
        }

        TEST_METHOD(StringTest_setUnquote)
        {
            String s = "xyz";

            s.setUnquote("\"abc\"");
            Assert::AreEqual("abc", s.cstr());

            // 二文字に満たない場合は空になる。
            s.setUnquote("a");
            Assert::AreEqual("", s.cstr());
        }

        TEST_METHOD(StringTest_clear)
        {
            String s = "abc";

            Assert::AreEqual("abc", s.cstr());
            s.clear();

            Assert::AreEqual("", s.cstr());
        }

        TEST_METHOD(StringTest_equalOperatorCString)
        {
            String s;
            const char* xs = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"; // 300 x's

            s = xs;
            Assert::AreNotEqual(xs, s.cstr());
            Assert::AreEqual(255, (int)strlen(s.cstr()));
        }

        TEST_METHOD(StringTest_equalOperatorStdString)
        {
            String s;
            const char* xs = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"; // 300 x's

            s = std::string(xs);
            Assert::AreNotEqual(xs, s.cstr());
            Assert::AreEqual(255, (int)strlen(s.cstr()));
        }

        TEST_METHOD(StringTest_cstr)
        {
            String s = "abc";

            Assert::AreEqual("abc", s.cstr());
            Assert::AreSame(*s.data, *s.cstr());
        }

        TEST_METHOD(StringTest_c_str)
        {
            String s = "abc";

            Assert::AreEqual("abc", s.c_str());
            Assert::AreSame(*s.data, *s.c_str());
        }

        TEST_METHOD(StringTest_str)
        {
            Assert::AreEqual("", String("").str().c_str());
            Assert::AreEqual("A", String("A").str().c_str());

            String s = "abc";

            Assert::AreEqual("abc", s.str().c_str());
            Assert::AreNotSame(*s.data, *s.str().c_str());
        }

        TEST_METHOD(StringTest_size)
        {
            Assert::AreEqual(0, (int)String("").size());
            Assert::AreEqual(3, (int)String("abc").size());
        }

        TEST_METHOD(StringTest_setFromString)
        {
            String s;

            s.setFromString("abc def");
            Assert::AreEqual("abc", s.data);

            s.setFromString("\"abc def\"");
            Assert::AreEqual("abc def", s.data);
        }

    };
}

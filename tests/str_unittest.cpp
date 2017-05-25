#include "stdafx.h"
#include "CppUnitTest.h"

#include "str.h"

using namespace str;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace strFixture
{
    TEST_CLASS(strFixture)
    {
    public:

        TEST_METHOD(strFixture_group_digits)
        {
            Assert::AreEqual("0", group_digits("0").c_str());
            Assert::AreEqual("1", group_digits("1").c_str());
            Assert::AreEqual("1.0", group_digits("1.0").c_str());
            Assert::AreEqual("1.1", group_digits("1.1").c_str());
            Assert::AreEqual("1.1234", group_digits("1.1234").c_str());
            Assert::AreEqual("123,456", group_digits("123456").c_str());
            Assert::AreEqual("123,456.789123", group_digits("123456.789123").c_str());

            Assert::AreEqual("1",             group_digits("1").c_str());
            Assert::AreEqual("11",            group_digits("11").c_str());
            Assert::AreEqual("111",           group_digits("111").c_str());
            Assert::AreEqual("1,111",         group_digits("1111").c_str());
            Assert::AreEqual("11,111",        group_digits("11111").c_str());
            Assert::AreEqual("111,111",       group_digits("111111").c_str());
            Assert::AreEqual("1,111,111",     group_digits("1111111").c_str());
            Assert::AreEqual("11,111,111",    group_digits("11111111").c_str());
            Assert::AreEqual("111,111,111",   group_digits("111111111").c_str());
            Assert::AreEqual("1,111,111,111", group_digits("1111111111").c_str());
        }

        TEST_METHOD(strFixture_split)
        {
            auto vec = split("a b c", " ");

            Assert::AreEqual(3, (int)vec.size());
            Assert::AreEqual("a", vec[0].c_str());
            Assert::AreEqual("b", vec[1].c_str());
            Assert::AreEqual("c", vec[2].c_str());
        }

        TEST_METHOD(strFixture_codepoint_to_utf8)
        {
            Assert::AreEqual(" ", codepoint_to_utf8(0x20).c_str());
            Assert::AreEqual("\xE3\x81\x82", codepoint_to_utf8(12354).c_str());
            Assert::AreEqual("\xf0\x9f\x92\xa9", codepoint_to_utf8(0x1f4a9).c_str()); // PILE OF POO
        }

        TEST_METHOD(strFixture_format)
        {
            Assert::AreEqual("a", format("a").c_str());
            Assert::AreEqual("a", format("%s", "a").c_str());
            Assert::AreEqual("1", format("%d", 1).c_str());
            Assert::AreEqual("12", format("%d%d", 1, 2).c_str());
        }

        TEST_METHOD(strFixture_contains)
        {
            Assert::IsTrue(str::contains("abc", "bc"));
            Assert::IsFalse(str::contains("abc", "d"));
            Assert::IsTrue(str::contains("abc", ""));
            Assert::IsTrue(str::contains("", ""));
            Assert::IsTrue(str::contains("abc", "abc"));
            Assert::IsFalse(str::contains("", "abc"));
        }

        TEST_METHOD(strFixture_replace_prefix)
        {
            Assert::AreEqual("", str::replace_prefix("", "", "").c_str());
            Assert::AreEqual("b", str::replace_prefix("", "", "b").c_str());
            Assert::AreEqual("", str::replace_prefix("", "a", "b").c_str());
            Assert::AreEqual("", str::replace_prefix("", "a", "").c_str());
            Assert::AreEqual("xbc", str::replace_prefix("abc", "a", "x").c_str());
            Assert::AreEqual("abc", str::replace_prefix("abc", "x", "x").c_str());
            Assert::AreEqual("xabc", str::replace_prefix("abc", "", "x").c_str());
            Assert::AreEqual("bc", str::replace_prefix("abc", "a", "").c_str());
        }

        TEST_METHOD(strFixture_replace_suffix)
        {
            Assert::AreEqual("", str::replace_suffix("", "", "").c_str());
            Assert::AreEqual("b", str::replace_suffix("", "", "b").c_str());
            Assert::AreEqual("", str::replace_suffix("", "a", "b").c_str());
            Assert::AreEqual("", str::replace_suffix("", "a", "").c_str());
            Assert::AreEqual("abx", str::replace_suffix("abc", "c", "x").c_str());
            Assert::AreEqual("abc", str::replace_suffix("abc", "x", "x").c_str());
            Assert::AreEqual("abcx", str::replace_suffix("abc", "", "x").c_str());
            Assert::AreEqual("ab", str::replace_suffix("abc", "c", "").c_str());
        }

        TEST_METHOD(strFixture_capitalize)
        {
            Assert::AreEqual("", str::capitalize("").c_str());
            Assert::AreEqual("A", str::capitalize("a").c_str());
            Assert::AreEqual("@", str::capitalize("@").c_str());
            Assert::AreEqual("Abc", str::capitalize("abc").c_str());
            Assert::AreEqual("Abc", str::capitalize("ABC").c_str());
            Assert::AreEqual("A@B", str::capitalize("a@b").c_str());
            Assert::AreEqual("Content-Type", str::capitalize("CONTENT-TYPE").c_str());
            Assert::AreEqual("Content-Type", str::capitalize("content-type").c_str());
            Assert::AreEqual("Content-Type", str::capitalize("Content-Type").c_str());
            Assert::AreEqual("��������������������", str::downcase("��������������������").c_str());
        }

        TEST_METHOD(strFixture_upcase)
        {
            Assert::AreEqual("", str::upcase("").c_str());
            Assert::AreEqual("A", str::upcase("a").c_str());
            Assert::AreEqual("@", str::upcase("@").c_str());
            Assert::AreEqual("ABC", str::upcase("abc").c_str());
            Assert::AreEqual("ABC", str::upcase("ABC").c_str());
            Assert::AreEqual("A@B", str::upcase("a@b").c_str());
            Assert::AreEqual("CONTENT-TYPE", str::upcase("CONTENT-TYPE").c_str());
            Assert::AreEqual("CONTENT-TYPE", str::upcase("content-type").c_str());
            Assert::AreEqual("CONTENT-TYPE", str::upcase("Content-Type").c_str());
            Assert::AreEqual("��������������������", str::downcase("��������������������").c_str());
        }

        TEST_METHOD(strFixture_downcase)
        {
            Assert::AreEqual("", str::downcase("").c_str());
            Assert::AreEqual("a", str::downcase("a").c_str());
            Assert::AreEqual("@", str::downcase("@").c_str());
            Assert::AreEqual("abc", str::downcase("abc").c_str());
            Assert::AreEqual("abc", str::downcase("ABC").c_str());
            Assert::AreEqual("a@b", str::downcase("a@b").c_str());
            Assert::AreEqual("content-type", str::downcase("CONTENT-TYPE").c_str());
            Assert::AreEqual("content-type", str::downcase("content-type").c_str());
            Assert::AreEqual("content-type", str::downcase("Content-Type").c_str());
            Assert::AreEqual("��������������������", str::downcase("��������������������").c_str());
        }

        TEST_METHOD(strFixture_is_prefix_of)
        {
            Assert::IsTrue(str::is_prefix_of("", ""));
            Assert::IsTrue(str::is_prefix_of("", "a"));
            Assert::IsTrue(str::is_prefix_of("a", "a"));
            Assert::IsTrue(str::is_prefix_of("a", "abc"));
            Assert::IsFalse(str::is_prefix_of("b", ""));
            Assert::IsFalse(str::is_prefix_of("b", "a"));
            Assert::IsFalse(str::is_prefix_of("b", "abc"));
            Assert::IsTrue(str::is_prefix_of("abc", "abc"));
            Assert::IsTrue(str::is_prefix_of("��", "����������"));
        }

        TEST_METHOD(strFixture_join)
        {
            Assert::AreEqual("", str::join("", {}).c_str());
            Assert::AreEqual("", str::join(",", {}).c_str());
            Assert::AreEqual("a,b", str::join(",", { "a", "b" }).c_str());
            Assert::AreEqual("ab", str::join("", { "a", "b" }).c_str());
            Assert::AreEqual("ab", str::join("", str::split("a,b", ",")).c_str());
        }

        TEST_METHOD(strFixture_extension_without_dot)
        {
            Assert::AreEqual("", str::extension_without_dot("").c_str());
            Assert::AreEqual("flv", str::extension_without_dot("test.flv").c_str());
            Assert::AreEqual("FLV", str::extension_without_dot("TEST.FLV").c_str());
            Assert::AreEqual("", str::extension_without_dot("test.").c_str());
            Assert::AreEqual("gz", str::extension_without_dot("hoge.tar.gz").c_str());
        }

    };
}

#include "stdafx.h"
#include "CppUnitTest.h"

#include "template.h"
#include "sstream.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TemplateFixture
{
    TEST_CLASS(TemplateFixture)
    {
    public:

        TemplateFixture()
            : temp("")
        {
        }

        Template temp;

        TEST_METHOD(TemplateFixture_readVariable)
        {
            StringStream in, out;

            in.writeString("servMgr.version}");
            in.rewind();
            temp.readVariable(in, &out, 0);
            Assert::AreEqual("v0.1218", out.str().substr(0, 7).c_str());
        }

        TEST_METHOD(TemplateFixture_writeVariable)
        {
            StringStream out;

            temp.writeVariable(out, "servMgr.version", 0);
            Assert::AreEqual("v0.1218", out.str().substr(0, 7).c_str());
        }

        TEST_METHOD(TemplateFixture_writeVariableUndefined)
        {
            StringStream out;

            temp.writeVariable(out, "hoge.fuga.piyo", 0);
            Assert::AreEqual("hoge.fuga.piyo", out.str().c_str());
        }

        TEST_METHOD(TemplateFixture_getIntVariable)
        {
            Assert::AreEqual(0, temp.getIntVariable("servMgr.version", 0));
        }

        TEST_METHOD(TemplateFixture_getBoolVariable)
        {
            Assert::AreEqual(false, (bool)temp.getIntVariable("servMgr.version", 0));
        }


        TEST_METHOD(TemplateFixture_readTemplate)
        {
            StringStream in, out;
            bool res;

            in.writeString("hoge");
            in.rewind();
            res = temp.readTemplate(in, &out, 0);
            Assert::IsFalse(res);
            Assert::AreEqual("hoge", out.str().c_str());
        }

        TEST_METHOD(TemplateFixture_getStringVariable)
        {
            Assert::AreEqual("1", temp.getStringVariable("TRUE", 0).c_str());
            Assert::AreEqual("0", temp.getStringVariable("FALSE", 0).c_str());
        }

        TEST_METHOD(TemplateFixture_evalStringLiteral)
        {
            Assert::AreEqual("abc", temp.evalStringLiteral("\"abc\"").c_str());
            Assert::AreEqual("", temp.evalStringLiteral("\"\"").c_str());
        }

        TEST_METHOD(TemplateFixture_readStringLiteral)
        {
            std::string lit, rest;
            std::tie(lit, rest) = temp.readStringLiteral("\"abc\"def");

            Assert::AreEqual("\"abc\"", lit.c_str());
            Assert::AreEqual("def", rest.c_str());
        }

        TEST_METHOD(TemplateFixture_tokenizeBinaryExpression)
        {
            auto tok = temp.tokenize("a==b");

            Assert::AreEqual(3, (int)tok.size());
            Assert::AreEqual("a", tok[0].c_str());
            Assert::AreEqual("==", tok[1].c_str());
            Assert::AreEqual("b", tok[2].c_str());
        }

        TEST_METHOD(TemplateFixture_tokenizeVariableExpression)
        {
            auto tok = temp.tokenize("a");

            Assert::AreEqual(1, (int)tok.size());
            Assert::AreEqual("a", tok[0].c_str());

            tok = temp.tokenize("!a");
            Assert::AreEqual(1, (int)tok.size());
            Assert::AreEqual("!a", tok[0].c_str());
        }

        TEST_METHOD(TemplateFixture_evalCondition)
        {
            Assert::IsTrue(temp.evalCondition("TRUE", 0));
            Assert::IsFalse(temp.evalCondition("FALSE", 0));
        }

        TEST_METHOD(TemplateFixture_evalCondition2)
        {
            Assert::IsTrue(temp.evalCondition("TRUE==TRUE", 0));
            Assert::IsFalse(temp.evalCondition("TRUE==FALSE", 0));
        }

        TEST_METHOD(TemplateFixture_evalCondition3)
        {
            Assert::IsFalse(temp.evalCondition("TRUE!=TRUE", 0));
            Assert::IsTrue(temp.evalCondition("TRUE!=FALSE", 0));
        }

        TEST_METHOD(TemplateFixture_readIfTrue)
        {
            StringStream in, out;

            in.writeString("TRUE}T{@else}F{@end}");
            in.rewind();
            temp.readIf(in, &out, 0);
            Assert::AreEqual("T", out.str().c_str());
        }

        TEST_METHOD(TemplateFixture_readIfFalse)
        {
            StringStream in, out;

            in.writeString("FALSE}T{@else}F{@end}");
            in.rewind();
            temp.readIf(in, &out, 0);
            Assert::AreEqual("F", out.str().c_str());
        }

        TEST_METHOD(TemplateFixture_readIfTrueWithoutElse)
        {
            StringStream in, out;

            in.writeString("TRUE}T{@end}");
            in.rewind();
            temp.readIf(in, &out, 0);
            Assert::AreEqual("T", out.str().c_str());
        }

        TEST_METHOD(TemplateFixture_readIfFalseWithoutElse)
        {
            StringStream in, out;

            in.writeString("FALSE}T{@end}");
            in.rewind();
            temp.readIf(in, &out, 0);
            Assert::AreEqual("", out.str().c_str());
        }

        TEST_METHOD(TemplateFixture_fragment)
        {
            StringStream in, out;
            bool res;

            in.writeString("hoge{@fragment a}fuga{@end}piyo");
            in.rewind();
            res = temp.readTemplate(in, &out, 0);
            Assert::IsFalse(res);
            Assert::AreEqual("hogefugapiyo", out.str().c_str());
        }

        TEST_METHOD(TemplateFixture_fragment2)
        {
            StringStream in, out;
            bool res;

            in.writeString("hoge{@fragment a}fuga{@end}piyo");
            in.rewind();
            temp.selectedFragment = "a";
            res = temp.readTemplate(in, &out, 0);
            Assert::IsFalse(res);
            Assert::AreEqual("fuga", out.str().c_str());
        }

        TEST_METHOD(TemplateFixture_fragment3)
        {
            StringStream in, out;
            bool res;

            in.writeString("hoge{@fragment a}fuga{@end}piyo");
            in.rewind();
            temp.selectedFragment = "b";
            res = temp.readTemplate(in, &out, 0);
            Assert::IsFalse(res);
            Assert::AreEqual("", out.str().c_str());
        }

        TEST_METHOD(TemplateFixture_variableInFragment)
        {
            StringStream in, out;
            bool res;

            in.writeString("{@fragment a}{$TRUE}{@end}{@fragment b}{$FALSE}{@end}");
            in.rewind();
            res = temp.readTemplate(in, &out, 0);
            Assert::IsFalse(res);
            Assert::AreEqual("10", out.str().c_str());
        }

        TEST_METHOD(TemplateFixture_variableInFragment2)
        {
            StringStream in, out;
            bool res;

            in.writeString("{@fragment a}{$TRUE}{@end}{@fragment b}{$FALSE}{@end}");
            in.rewind();
            temp.selectedFragment = "a";
            res = temp.readTemplate(in, &out, 0);
            Assert::IsFalse(res);
            Assert::AreEqual("1", out.str().c_str());
        }

        TEST_METHOD(TemplateFixture_variableInFragment3)
        {
            StringStream in, out;
            bool res;

            in.writeString("{@fragment a}{$TRUE}{@end}{$FALSE}");
            in.rewind();
            temp.selectedFragment = "a";
            res = temp.readTemplate(in, &out, 0);
            Assert::IsFalse(res);
            Assert::AreEqual("1", out.str().c_str());
        }

    };
}

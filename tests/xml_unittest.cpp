#include "stdafx.h"
#include "CppUnitTest.h"

#include "sstream.h"
#include "xml.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace XMLFixture
{
    TEST_CLASS(XMLFixture)
    {
    public:

        TEST_METHOD(read)
        {
            StringStream mem;
            mem.writeString("<br/>");
            mem.rewind();

            XML xml;
            Assert::IsTrue(xml.root == NULL);

            xml.read(mem);
            Assert::IsTrue(xml.root != NULL);

            Assert::AreEqual("br", xml.root->getName());
        }

        // タグ名はフォーマット文字列として解釈されてはならない。
        TEST_METHOD(readCrash)
        {
            StringStream mem;
            mem.writeString("<%s%s%s%s%s%s%s%s%s%s%s%s/>");
            mem.rewind();

            XML xml;
            xml.read(mem);

            Assert::IsTrue(xml.root != NULL);
            Assert::AreEqual("%s%s%s%s%s%s%s%s%s%s%s%s", xml.root->getName());
        }

    };
}

#include "stdafx.h"
#include "CppUnitTest.h"

#include "sys.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GlobalFunctionsTest
{
    TEST_CLASS(GlobalFunctionsTest)
    {
    public:

        TEST_METHOD(GlobalFunctionsTest_trimstr_nullCase)
        {
            char str[100] = "";
            Assert::AreEqual("", trimstr(str));
        }

        TEST_METHOD(GlobalFunctionsTest_trimstr_noSpace)
        {
            char str[100] = "word";
            Assert::AreEqual("word", trimstr(str));
        }

        TEST_METHOD(GlobalFunctionsTest_trimstr_space)
        {
            char str[100] = " word ";
            Assert::AreEqual("word", trimstr(str));
        }

        TEST_METHOD(GlobalFunctionsTest_trimstr_tab)
        {
            char str[100] = "\tword\t";
            Assert::AreEqual("word", trimstr(str));
        }

        // �󔒂�������Ȃ镶�����n���ƁA������̐擪���ȑO�̃������ɃA�N
        // �Z�X����o�O���������B
        TEST_METHOD(GlobalFunctionsTest_trimstr_letsTryToSmashStack)
        {
            char changeMe[2] = { 'A','\t' };
            char str[100] = " ";

            Assert::AreEqual("", trimstr(str));
            Assert::AreEqual('A', changeMe[0]);
            Assert::AreEqual('\t', changeMe[1]);
        }

    };
}

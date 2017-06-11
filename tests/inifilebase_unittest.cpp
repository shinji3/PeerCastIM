#include "stdafx.h"
#include "CppUnitTest.h"

#include "inifile.h"
#include "sstream.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace IniFileBaseFixture
{
    TEST_CLASS(IniFileBaseFixture)
    {
    public:

    IniFileBaseFixture()
       : ini(mem)
    {
    }

    StringStream mem;
    IniFileBase ini;

TEST_METHOD(IniFileBaseFixture_initialState)
{
    Assert::AreEqual("", ini.currLine);
    Assert::AreEqual(nullptr, ini.nameStr);
    Assert::AreEqual(nullptr, ini.valueStr);
}

TEST_METHOD(IniFileBaseFixture_readNext_eof)
{
    mem.str("");
    Assert::IsFalse(ini.readNext());
}

TEST_METHOD(IniFileBaseFixture_emptyLine)
{
    mem.str("\n");
    Assert::IsTrue(ini.readNext());
    Assert::AreEqual("", ini.getName());
    Assert::AreEqual("", ini.getStrValue());
}

TEST_METHOD(IniFileBaseFixture_readNext_assignment)
{
    mem.str("x = 1\n");
    Assert::IsTrue(ini.readNext());
    Assert::AreEqual("x", ini.getName());
    Assert::AreEqual("1", ini.getStrValue());
    Assert::AreEqual(1, ini.getIntValue());
    Assert::AreEqual(true, ini.getBoolValue());
}

TEST_METHOD(IniFileBaseFixture_namesAndValuesAreTrimmed)
{
    mem.str("\ty   =    2   \n");
    Assert::IsTrue(ini.readNext());
    Assert::AreEqual("y", ini.getName());
    Assert::AreEqual("2", ini.getStrValue());
}

TEST_METHOD(IniFileBaseFixture_getBoolValue)
{
    mem.str("x = 0\n");
    Assert::IsTrue(ini.readNext());
    Assert::AreEqual(false, ini.getBoolValue());
}

TEST_METHOD(IniFileBaseFixture_readNext_assignmentWithoutValue)
{
    mem.str("x = \n");
    Assert::IsTrue(ini.readNext());
    Assert::AreEqual("x", ini.getName());
    Assert::AreEqual("", ini.getStrValue());
    Assert::AreEqual(0, ini.getIntValue());
    Assert::AreEqual(false, ini.getBoolValue());
}

TEST_METHOD(IniFileBaseFixture_readNext_sectionHeader)
{
    mem.str("[Section]\n");
    Assert::IsTrue(ini.readNext());
    Assert::AreEqual("[Section]", ini.getName());
    Assert::AreEqual("", ini.getStrValue());
}

// writing


TEST_METHOD(IniFileBaseFixture_writeSection)
{
    ini.writeSection("Section");
    Assert::AreEqual("\r\n[Section]\r\n", mem.str());
}

TEST_METHOD(IniFileBaseFixture_writeIntValue)
{
    ini.writeIntValue("x", 1);
    Assert::AreEqual("x = 1\r\n", mem.str());
}

TEST_METHOD(IniFileBaseFixture_writeStrValue)
{
    ini.writeStrValue("y", "foo bar baz");
    Assert::AreEqual("y = foo bar baz\r\n", mem.str());
}

TEST_METHOD(IniFileBaseFixture_letsTryToSmashStack)
{
    ini.writeStrValue("veryLongString", "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    Assert::AreEqual("veryLongString = AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\n", mem.str());
}

TEST_METHOD(IniFileBaseFixture_writeBoolValue_true)
{
    ini.writeBoolValue("z", 1);
    Assert::AreEqual("z = Yes\r\n", mem.str());
}

TEST_METHOD(IniFileBaseFixture_writeBoolValue_false)
{
    ini.writeBoolValue("z", 0);
    Assert::AreEqual("z = No\r\n", mem.str());
}

TEST_METHOD(IniFileBaseFixture_writeLine)
{
    ini.writeLine("[End]");
    Assert::AreEqual("[End]\r\n", mem.str());
}

    };
}

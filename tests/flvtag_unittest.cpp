#include "stdafx.h"
#include "CppUnitTest.h"

#include "flv.h"
#include "sstream.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FLVTagFixture
{
    TEST_CLASS(FLVTagFixture)
    {
    public:

        FLVTagFixture()
            :data{ // a Script tag
            0x12,0x00,0x01,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x0A,0x6F,0x6E,0x4D,0x65,0x74,0x61,0x44,0x61,0x74,0x61,
            0x08,0x00,0x00,0x00,0x0C,0x00,0x08,0x64,0x75,0x72,0x61,0x74,0x69,0x6F,0x6E,0x00,0x40,0xB8,0x28,0xC0,0x00,0x00,0x00,0x00,
            0x00,0x05,0x77,0x69,0x64,0x74,0x68,0x00,0x40,0x94,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x68,0x65,0x69,0x67,0x68,0x74,
            0x00,0x40,0x86,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x0D,0x76,0x69,0x64,0x65,0x6F,0x64,0x61,0x74,0x61,0x72,0x61,0x74,0x65,
            0x00,0x40,0x72,0x0D,0xEC,0x00,0x00,0x00,0x00,0x00,0x0C,0x76,0x69,0x64,0x65,0x6F,0x63,0x6F,0x64,0x65,0x63,0x69,0x64,0x00,
            0x40,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0D,0x61,0x75,0x64,0x69,0x6F,0x64,0x61,0x74,0x61,0x72,0x61,0x74,0x65,0x00,
            0x40,0x59,0x46,0xE0,0x00,0x00,0x00,0x00,0x00,0x0F,0x61,0x75,0x64,0x69,0x6F,0x73,0x61,0x6D,0x70,0x6C,0x65,0x72,0x61,0x74,
            0x65,0x00,0x40,0xE7,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x61,0x75,0x64,0x69,0x6F,0x73,0x61,0x6D,0x70,0x6C,0x65,0x73,
            0x69,0x7A,0x65,0x00,0x40,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x73,0x74,0x65,0x72,0x65,0x6F,0x01,0x01,0x00,0x0C,
            0x61,0x75,0x64,0x69,0x6F,0x63,0x6F,0x64,0x65,0x63,0x69,0x64,0x00,0x40,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x65,
            0x6E,0x63,0x6F,0x64,0x65,0x72,0x02,0x00,0x0D,0x4C,0x61,0x76,0x66,0x35,0x37,0x2E,0x36,0x36,0x2E,0x31,0x30,0x32,0x00,0x08,
            0x66,0x69,0x6C,0x65,0x73,0x69,0x7A,0x65,0x00,0x41,0xB2,0xD7,0x9D,0x35,0x00,0x00,0x00,0x00,0x00
        }
        {
        }

        FLVTag tag;
        unsigned char data[283];

        TEST_METHOD(FLVTagFixture_initialState)
        {
            Assert::AreEqual(0, tag.size);
            Assert::AreEqual(0, tag.packetSize);
            Assert::AreEqual((int)FLVTag::T_UNKNOWN, (int)tag.type);
            Assert::IsNull(tag.data);
            Assert::IsNull(tag.packet);
        }

        TEST_METHOD(FLVTagFixture_timestamp)
        {
            StringStream mem(std::string((char*)data, (char*)data + 283));

            Assert::AreEqual(283, mem.getLength());

            tag.read(mem);

            Assert::AreEqual(283, mem.getPosition());

            Assert::AreEqual(273, tag.size);
            Assert::AreEqual(288, tag.packetSize);

            Assert::AreEqual(0, tag.getTimestamp());

            tag.setTimestamp(0x12345678);

            Assert::AreEqual(0x12345678, tag.getTimestamp());

            Assert::IsFalse(tag.isKeyFrame());
            Assert::AreEqual((int)FLVTag::T_SCRIPT, (int)tag.type);
        }

    };
}

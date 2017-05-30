#include "stdafx.h"
#include "CppUnitTest.h"

#include "mapper.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FileSystemMapperFixture
{
    TEST_CLASS(FileSystemMapperFixture)
    {
    public:

        TEST_METHOD(FileSystemMapperFixture_realPath)
        {
            Assert::AreEqual("C:\\", FileSystemMapper::realPath("/").c_str());
            Assert::AreEqual("C:\\tmp", FileSystemMapper::realPath("/tmp").c_str());
            Assert::AreEqual("C:\\root", FileSystemMapper::realPath("/root").c_str());
            Assert::AreEqual("C:\\root", FileSystemMapper::realPath("/tmp/../root").c_str());
            Assert::AreEqual("C:\\nonexistent", FileSystemMapper::realPath("/nonexistent").c_str());
        }

    };
}

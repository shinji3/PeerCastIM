#include "stdafx.h"
#include "CppUnitTest.h"
#include "unittest.h"

#include "sys.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace tests
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			// TODO: �e�X�g �R�[�h�������ɑ}�����܂�
            String str;
            Assert::IsTrue(str.isEmpty());
		}

	};
}
#include "stdafx.h"
#include "CppUnitTest.h"

#include "public.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PublicControllerFixture
{
    TEST_CLASS(PublicControllerFixture)
    {
    public:

        TEST_METHOD(PublicControllerFixture_formatUptime)
        {
            Assert::AreEqual("00:00", PublicController::formatUptime(0).c_str());
            Assert::AreEqual("00:00", PublicController::formatUptime(1).c_str());
            Assert::AreEqual("00:00", PublicController::formatUptime(59).c_str());
            Assert::AreEqual("00:01", PublicController::formatUptime(60).c_str());
            Assert::AreEqual("00:59", PublicController::formatUptime(3600 - 1).c_str());
            Assert::AreEqual("01:00", PublicController::formatUptime(3600).c_str());
            Assert::AreEqual("99:59", PublicController::formatUptime(100 * 3600 - 1).c_str());
            Assert::AreEqual("100:00", PublicController::formatUptime(100 * 3600).c_str());
        }

        TEST_METHOD(PublicControllerFixture_acceptableLanguages_nullCase)
        {
            std::vector<std::string> langs;
            langs = PublicController::acceptableLanguages("");
            Assert::AreEqual(0, (int)langs.size());
        }

        TEST_METHOD(PublicControllerFixture_acceptableLanguages1)
        {
            std::vector<std::string> langs;
            langs = PublicController::acceptableLanguages("fr-CA");
            Assert::AreEqual(1, (int)langs.size());
            Assert::AreEqual("fr-CA", langs[0].c_str());
        }

        TEST_METHOD(PublicControllerFixture_acceptableLanguages2)
        {
            std::vector<std::string> langs;
            langs = PublicController::acceptableLanguages("ja,en;q=0.5");
            Assert::AreEqual(2, (int)langs.size());
            Assert::AreEqual("ja", langs[0].c_str());
            Assert::AreEqual("en", langs[1].c_str());
        }

        TEST_METHOD(PublicControllerFixture_acceptableLanguages3)
        {
            std::vector<std::string> langs;
            langs = PublicController::acceptableLanguages("ja;q=0.5,en");
            Assert::AreEqual(2, (int)langs.size());
            Assert::AreEqual("en", langs[0].c_str());
            Assert::AreEqual("ja", langs[1].c_str());
        }

    };
}

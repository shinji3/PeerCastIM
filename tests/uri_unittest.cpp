// URI �N���X�̃e�X�g�B

#include "stdafx.h"
#include "CppUnitTest.h"

#include "uri.h"
#include "LUrlParser.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace URIFixture
{
    TEST_CLASS(URIFixture)
    {
    public:

        TEST_METHOD(URIFixture_httpScheme)
        {
            URI u("http://www.example.com");

            Assert::IsTrue(u.isValid());
            Assert::AreEqual("http", u.scheme().c_str());
            Assert::AreEqual("www.example.com", u.host().c_str());
            Assert::AreEqual(80, u.port()); // �|�[�g�w�肪�Ȃ��ꍇ�̓X�L�[���̃f�t�H���g�|�[�g�B
            Assert::AreEqual("/", u.path().c_str()); // �p�X���ȗ�����Ă���ꍇ�� "/" �ɂȂ�B
        }

        TEST_METHOD(URIFixture_httpSchemeWithPortQueryAndFragment)
        {
            URI u("http://localhost:7144/html/en/index.html?name=%E4%BA%88%E5%AE%9A%E5%9C%B0#top");

            Assert::IsTrue(u.isValid());
            Assert::AreEqual("http", u.scheme().c_str());
            Assert::AreEqual("localhost", u.host().c_str());
            Assert::AreEqual(7144, u.port());
            Assert::AreEqual("/html/en/index.html", u.path().c_str());
            Assert::AreEqual("name=%E4%BA%88%E5%AE%9A%E5%9C%B0", u.query().c_str()); // �����I�� unescape �͂���Ȃ��B
            Assert::AreEqual("top", u.fragment().c_str());
        }

        TEST_METHOD(URIFixture_ftpScheme)
        {
            URI u("ftp://user:pass@localhost/pub/file.bin");
            Assert::IsTrue(u.isValid());
            Assert::AreEqual("ftp", u.scheme().c_str());
            Assert::AreEqual("user:pass", u.user_info().c_str());
            Assert::AreEqual("localhost", u.host().c_str());
            Assert::AreEqual("/pub/file.bin", u.path().c_str());
        }

        TEST_METHOD(URIFixture_invalidURI)
        {
            URI u("hoge");

            Assert::IsFalse(u.isValid());
            Assert::AreEqual("", u.scheme().c_str());
        }

        TEST_METHOD(URIFixture_emptyURI)
        {
            bool e = false;
            try
            {
                URI("");
            }
            catch (std::runtime_error)
            {
                e = true;
            }
            Assert::AreNotEqual(e, true);

            URI u("");
            Assert::IsFalse(u.isValid());
        }

        // TEST_METHOD(URIFixture_mailtoScheme)
        // {
        //     URI u("mailto:webmaster@example.com");
        //     Assert::IsTrue(u.isValid());
        //     Assert::AreEqual("mailto", u.scheme().c_str());
        //     Assert::AreEqual("webmaster@example.com", u.path().c_str());
        //     Assert::AreEqual("", u.host().c_str());
        // }

        // mailto�X�L�[���ɂ͑Ή����Ȃ��B
        TEST_METHOD(URIFixture_mailtoScheme)
        {
            URI u("mailto:webmaster@example.com");
            Assert::IsFalse(u.isValid());
        }

        // ����URL�͎g���Ȃ��B
        TEST_METHOD(URIFixture_relativeURI)
        {
            bool e = false;
            try
            {
                URI("/index.html");
            }
            catch (std::runtime_error)
            {
                e = true;
            }
            Assert::AreNotEqual(e, true);

            URI u("/index.html");
            Assert::IsFalse(u.isValid());
        }

    };
}

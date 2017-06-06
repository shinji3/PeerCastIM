#include "stdafx.h"
#include "CppUnitTest.h"

#include "env.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace EnvironmentFixture
{
    TEST_CLASS(EnvironmentFixture)
    {
    public:

        Environment env;

        TEST_METHOD(EnvironmentFixture_initialState)
        {
            Assert::AreEqual(0, (int)env.keys().size());
            Assert::IsTrue(env.env());
        }

        TEST_METHOD(EnvironmentFixture_constructor)
        {
            Environment e(environ);

            Assert::AreNotEqual(0, (int)e.size());
        }

        TEST_METHOD(EnvironmentFixture_hasKey)
        {
            Assert::IsFalse(env.hasKey("my-key"));

            env.set("my-key", "");
            Assert::IsTrue(env.hasKey("my-key"));
        }

        TEST_METHOD(EnvironmentFixture_hasKey_nullCase)
        {
            Assert::IsFalse(env.hasKey(""));
        }

        TEST_METHOD(EnvironmentFixture_setAndGet)
        {
            Assert::AreEqual("", env.get("my-key").c_str());

            env.set("my-key", "my-value");
            Assert::AreEqual("my-value", env.get("my-key").c_str());
        }

        TEST_METHOD(EnvironmentFixture_env_nullCase)
        {
            Assert::AreEqual(NULL, env.env()[0]);
        }

        TEST_METHOD(EnvironmentFixture_env)
        {
            env.set("key1", "value1");
            env.set("key2", "value2");
            env.set("key3", "value3");

            char const ** e = env.env();
            Assert::AreEqual("key1=value1", e[0]);
            Assert::AreEqual("key2=value2", e[1]);
            Assert::AreEqual("key3=value3", e[2]);
        }

        TEST_METHOD(EnvironmentFixture_keys)
        {
            env.set("key1", "value1");
            env.set("key2", "value2");
            env.set("key3", "value3");

            auto keys = env.keys();
            Assert::AreEqual(3, (int)keys.size());
            Assert::AreEqual("key1", keys[0].c_str());
            Assert::AreEqual("key2", keys[1].c_str());
            Assert::AreEqual("key3", keys[2].c_str());
        }

        TEST_METHOD(EnvironmentFixture_size)
        {
            env.set("key1", "value1");
            env.set("key2", "value2");
            env.set("key3", "value3");

            Assert::AreEqual(3, (int)env.size());
        }

        TEST_METHOD(EnvironmentFixture_unset)
        {
            env.set("key1", "value1");
            env.set("key2", "value2");
            env.set("key3", "value3");

            Assert::AreEqual(3, (int)env.size());

            env.unset("key2");
            Assert::AreEqual(2, (int)env.size());
        }

    };
}

#include "env.h"
#include "str.h"

#include <stdlib.h>
#include <assert.h>

Environment::Environment()
    : m_env(NULL)
{
}

Environment::Environment(char *src[])
    : Environment()
{
    for (int i = 0; src[i] != NULL; i++)
        m_vars.push_back(src[i]);
}

Environment::~Environment()
{
    if (m_env)
        free(m_env);
}

void Environment::set(const std::string& key, const std::string& value)
{
    for (auto& entry : m_vars)
    {
        if (str::is_prefix_of(key + "=", entry))
        {
            entry = value;
            return;
        }
    }

    m_vars.push_back(key + "=" + value);
}

void Environment::unset(const std::string& key)
{
    for (auto it = m_vars.begin(); it != m_vars.end(); it++)
    {
        if (str::is_prefix_of(key+"=", *it))
        {
            m_vars.erase(it);
            return;
        }
    }
}

std::string Environment::get(const std::string& key) const
{
    for (auto& entry : m_vars)
    {
        if (str::is_prefix_of(key + "=", entry))
            return str::replace_prefix(entry, key + "=", "");
    }
    return "";
}

bool Environment::hasKey(const std::string& key) const
{
    for (auto& entry : m_vars)
    {
        if (str::is_prefix_of(key + "=", entry))
            return true;
    }
    return false;
}

std::vector<std::string> Environment::keys() const
{
    std::vector<std::string> res;

    for (auto& entry : m_vars)
    {
        assert(entry.find("=") != std::string::npos);
        res.push_back(entry.substr(0, entry.find("=")));
    }
    return res;
}

char const ** Environment::env()
{
    m_env = (decltype(m_env)) realloc(m_env, (m_vars.size()+1) * sizeof(char *));
    for (size_t i = 0; i < m_vars.size(); i++)
        m_env[i] = m_vars[i].c_str();
    m_env[m_vars.size()] = NULL;
    return m_env;
}

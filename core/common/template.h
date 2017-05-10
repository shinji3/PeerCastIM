// ------------------------------------------------
// File : template.h
// Date: 4-apr-2002
// Author: giles
// Desc:
//
// (c) 2002 peercast.org
// ------------------------------------------------
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ------------------------------------------------

#ifndef _TEMPLATE_H
#define _TEMPLATE_H

#include <list>
#include "sys.h"
#include "stream.h"
#include "json.hpp"

using json = nlohmann::json;

// HTML �e���v���[�g�V�X�e��
class Template
{
public:

    class Scope
    {
    public:
        virtual bool writeVariable(Stream &, const String &, int) = 0;
    };

    enum
    {
        TMPL_UNKNOWN,
        TMPL_LOOP,
        TMPL_IF,
        TMPL_ELSE,
        TMPL_END,
        TMPL_FRAGMENT,
        TMPL_FOREACH
    };

    Template(const char* args = NULL)
        : currentElement(json::object({}))
    {
        if (args)
            tmplArgs = _strdup(args);
        else
            tmplArgs = NULL;
    }

    Template(const std::string& args)
        : currentElement(json::object({}))
    {
        tmplArgs = _strdup(args.c_str());
    }

    ~Template()
    {
        if (tmplArgs)
            free(tmplArgs);
    }

    Template& prependScope(Scope& scope)
    {
        m_scopes.push_front(&scope);
        return *this;
    }

    bool inSelectedFragment()
    {
        if (selectedFragment.empty())
            return true;
        else
            return selectedFragment == currentFragment;
    }

    // �ϐ�
    void    writeVariable(Stream &, const String &, int);
    void    writeGlobalVariable(Stream &, const String &, int);
    int     getIntVariable(const String &, int);
    bool    getBoolVariable(const String &, int);

    // �f�B���N�e�B�u�̎��s
    int     readCmd(Stream &, Stream *, int);
    void    readIf(Stream &, Stream *, int);
    void    readLoop(Stream &, Stream *, int);
    void    readForeach(Stream &, Stream *, int);
    void    readFragment(Stream &, Stream *, int);

    void    readVariable(Stream &, Stream *, int);
    void    readVariableJavaScript(Stream &in, Stream *outp, int loop);
    void    readVariableRaw(Stream &in, Stream *outp, int loop);
    bool    readTemplate(Stream &, Stream *, int);
    bool    writeObjectProperty(Stream& s, const String& varName, json::object_t object);
    json::array_t evaluateCollectionVariable(String& varName);

    bool    evalCondition(const std::string& cond, int loop);
    std::vector<std::string> tokenize(const std::string& input);
    std::pair<std::string,std::string> readStringLiteral(const std::string& input);
    std::string evalStringLiteral(const std::string& input);
    std::string getStringVariable(const std::string& varName, int loop);

    char * tmplArgs;
    std::string selectedFragment;
    std::string currentFragment;
    json currentElement;

    std::list<Scope*> m_scopes;
};

#include "http.h"

// HTTP ���N�G�X�g��ϐ��Ƃ��ăG�N�X�|�[�g����X�R�[�v
class HTTPRequestScope : public Template::Scope
{
public:
    HTTPRequestScope(const HTTPRequest& aRequest)
        : m_request(aRequest)
    {
    }

    bool writeVariable(Stream &, const String &, int) override;

    const HTTPRequest& m_request;
};

#endif

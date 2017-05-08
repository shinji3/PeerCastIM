// ------------------------------------------------
// File : playlist.cpp
// Date: 4-apr-2002
// Author: giles
//
// (c) 2002 peercast.org
//
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

#include "playlist.h"
#include "servmgr.h"
#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif

// -----------------------------------
#define isHTMLSPECIAL(a) ((a == '&') || (a == '\"') || (a == '\'') || (a == '<') || (a == '>'))
static void SJIStoSJISSAFE(char *string, size_t size)
{
    size_t pos;
    for(pos = 0;
        (string[pos] != '\0') && (pos < size);
        ++pos)
    {
        if(isHTMLSPECIAL(string[pos]))
            string[pos] = ' ';
    }
}

// -----------------------------------
static void WriteASXInfo(Stream &out, String &title, String &contacturl, String::TYPE tEncoding = String::T_UNICODESAFE) //JP-MOD
{
    if(!title.isEmpty())
    {
        String titleEncode;
        titleEncode = title;
        titleEncode.convertTo(tEncoding);
        if(tEncoding == String::T_SJIS)
            SJIStoSJISSAFE(titleEncode.cstr(), String::MAX_LEN);
        out.writeLineF("<TITLE>%s</TITLE>", titleEncode.cstr());
    }

    if(!contacturl.isEmpty())
    {
        String contacturlEncode;
        contacturlEncode = contacturl;
        contacturlEncode.convertTo(tEncoding);
        if(tEncoding == String::T_SJIS)
            SJIStoSJISSAFE(contacturlEncode.cstr(), String::MAX_LEN);
        out.writeLineF("<MOREINFO HREF = \"%s\" />", contacturlEncode.cstr());
    }
}

// -----------------------------------
void PlayList::readASX(Stream &in)
{
    LOG_DEBUG("Reading ASX");
    XML xml;

    try
    {
        xml.read(in);
    }catch (StreamException &) {} // TODO: eof is NOT handled properly in sockets - always get error at end

    if (xml.root)
    {
        XML::Node *n = xml.root->child;
        while (n)
        {
            if (stricmp("entry", n->getName())==0)
            {
                XML::Node *rf = n->findNode("ref");
                if (rf)
                {
                    char *hr = rf->findAttr("href");
                    if (hr)
                    {
                        addURL(hr, "", "");
                        //LOG("asx url %s", hr);
                    }
                }
            }
            n=n->sibling;
        }
    }
}

// -----------------------------------
void PlayList::readSCPLS(Stream &in)
{
    char tmp[256];
    while (in.readLine(tmp, sizeof(tmp)))
    {
        if (strnicmp(tmp, "file", 4)==0)
        {
            char *p = strstr(tmp, "=");
            if (p)
                addURL(p+1, "", "");
        }
    }
}

// -----------------------------------
void PlayList::readPLS(Stream &in)
{
    char tmp[256];
    while (in.readLine(tmp, sizeof(tmp)))
    {
        if (tmp[0] != '#')
            addURL(tmp, "", "");
    }
}

// -----------------------------------
void PlayList::writeSCPLS(Stream &out)
{
    out.writeLine("[playlist]");
    out.writeLine("");
    out.writeLineF("NumberOfEntries=%d", numURLs);

    for (int i=0; i<numURLs; i++)
    {
        out.writeLineF("File%d=%s", i+1, urls[i].cstr());
        out.writeLineF("Title%d=%s", i+1, titles[i].cstr());
        out.writeLineF("Length%d=-1", i+1);
    }
    out.writeLine("Version=2");
}

// -----------------------------------
void PlayList::writePLS(Stream &out)
{
    for (int i=0; i<numURLs; i++)
        out.writeLineF("%s", urls[i].cstr());
}

// -----------------------------------
void PlayList::writeRAM(Stream &out)
{
    for (int i=0; i<numURLs; i++)
        out.writeLineF("%s", urls[i].cstr());
}

// -----------------------------------
void PlayList::writeASX(Stream &out)
{
    out.writeLine("<ASX Version=\"3.0\">");

    String::TYPE tEncoding = String::T_SJIS;
    if (servMgr->asxDetailedMode == 2)
    {
        out.writeLine("<PARAM NAME = \"Encoding\" VALUE = \"utf-8\" />"); //JP-MOD Memo: UTF-8 cannot be used in some recording software.
        tEncoding = String::T_UNICODESAFE;
    }

    if (servMgr->asxDetailedMode)
        WriteASXInfo(out, titles[0], contacturls[0], tEncoding); //JP-MOD

    for (int i = 0; i < numURLs; i++)
    {
        out.writeLine("<ENTRY>");
        if (servMgr->asxDetailedMode)
            WriteASXInfo(out, titles[i], contacturls[i], tEncoding); //JP-MOD
        out.writeLineF("<REF href = \"%s\" />", urls[i].cstr());
        out.writeLine("</ENTRY>");
    }
    out.writeLine("</ASX>");
}

// -----------------------------------
void PlayList::addChannel(const char *path, ChanInfo &info)
{
    String url;

    char idStr[64];

    info.id.toStr(idStr);
    char *nid = info.id.isSet()?idStr:info.name.cstr();

    sprintf(url.cstr(), "%s/stream/%s%s", path, nid, info.getTypeExt());
    addURL(url.cstr(), info.name, info.url);
}

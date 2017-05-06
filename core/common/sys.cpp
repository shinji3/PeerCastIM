// ------------------------------------------------
// File : sys.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      Sys is a base class for all things systemy, like starting threads, creating sockets etc..
//      Lock is a very basic cross platform CriticalSection class
//      SJIS-UTF8 conversion by ????
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

#include "common.h"
#include "sys.h"
#include "socket.h"
#include "gnutella.h"
#include "servmgr.h" //JP-EX
#ifdef WIN32
#include "utf8.h" //JP-Patch
#endif
#include <stdlib.h>
#include <time.h>
#include "jis.h"
#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif

// -----------------------------------
const char *LogBuffer::logTypes[]=
{
    "",
    "DBUG",
    "EROR",
    "GNET",
    "CHAN",
};

// -----------------------------------
// base64 encode/decode taken from ices2 source.. 
static char base64table[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
    'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
    'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
    'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
};
#if 0
// -----------------------------------
static char *util_base64_encode(char *data)
{
    int len = strlen(data);
    char *out = malloc(len*4/3 + 4);
    char *result = out;
    int chunk;

    while(len > 0) {
        chunk = (len >3)?3:len;
        *out++ = base64table[(*data & 0xFC)>>2];
        *out++ = base64table[((*data & 0x03)<<4) | ((*(data+1) & 0xF0) >> 4)];
        switch(chunk) {
            case 3:
                *out++ = base64table[((*(data+1) & 0x0F)<<2) | ((*(data+2) & 0xC0)>>6)];
                *out++ = base64table[(*(data+2)) & 0x3F];
                break;
            case 2:
                *out++ = base64table[((*(data+1) & 0x0F)<<2)];
                *out++ = '=';
                break;
            case 1:
                *out++ = '=';
                *out++ = '=';
                break;
        }
        data += chunk;
        len -= chunk;
    }

    return result;
}
#endif

// -----------------------------------
static char *util_base64_decode(char *input)
{
    return NULL;
}





// ------------------------------------------
Sys::Sys()
{
    idleSleepTime = 10;
    logBuf = new LogBuffer(1000, 100);
    numThreads=0;
}
// ------------------------------------------
void Sys::sleepIdle()
{
    sleep(idleSleepTime);
}

// -----------------------------------
char *trimstr(char *s1)
{
    while (*s1)
    {
        if ((*s1 == ' ') || (*s1 == '\t'))
            s1++;
        else
            break;

    }

    char *s = s1;

    if(strlen(s1) > 0) {
        s1 = s1+strlen(s1);

        while (*--s1)
            if ((*s1 != ' ') && (*s1 != '\t'))
                break;

        s1[1] = 0;
    }
    return s;
}

// -----------------------------------
char *stristr(const char *s1, const char *s2)
{
    while (*s1)
    {
        if (TOUPPER(*s1) == TOUPPER(*s2))
        {
            const char *c1 = s1;
            const char *c2 = s2;

            while (*c1 && *c2)
            {
                if (TOUPPER(*c1) != TOUPPER(*c2))
                    break;
                c1++;
                c2++;
            }
            if (*c2==0)
                return (char *)s1;
        }

        s1++;
    }
    return NULL;
}
// -----------------------------------
void LogBuffer::write(const char *str, TYPE t)
{
    lock.on();

    size_t len = strlen(str);
    int cnt=0;
    while (len)
    {
        size_t rlen = len;
        if (rlen > (lineLen-1))
            rlen = lineLen-1;

        int i = currLine % maxLines;
        int bp = i*lineLen;
        strncpy(&buf[bp], str, rlen);
        buf[bp+rlen] = 0;
        if (cnt==0)
        {
            times[i] = sys->getTime();
            types[i] = t;
        }else
        {
            times[i] = 0;
            types[i] = T_NONE;
        }
        currLine++;

        str += rlen;
        len -= rlen;
        cnt++;
    }

    lock.off();
}

// -----------------------------------
const char *getCGIarg(const char *str, const char *arg)
{
    if (!str)
        return NULL;

    const char *s = strstr(str, arg);

    if (!s)
        return NULL;

    s += strlen(arg);

    return s;
}

// -----------------------------------
bool cmpCGIarg(const char *str, const char *arg, const char *value)
{
    if ((!str) || (!strlen(value)))
        return false;

    if (strnicmp(str, arg, strlen(arg)) == 0)
    {

        str += strlen(arg);

        return strncmp(str, value, strlen(value))==0;
    }else
        return false;
}
// -----------------------------------
bool hasCGIarg(const char *str, const char *arg)
{
    if (!str)
        return false;

    const char *s = strstr(str, arg);

    if (!s)
        return false;

    return true;
}

// ---------------------------
void LogBuffer::dumpHTML(Stream &out)
{
    WLockBlock lb(&lock);
    lb.on();

    unsigned int nl = currLine;
    unsigned int sp = 0;
    if (nl > maxLines)
    {
        nl = maxLines-1;
        sp = (currLine+1)%maxLines;
    }

    String tim, str;
    if (nl)
    {
        for (unsigned int i=0; i<nl; i++)
        {
            unsigned int bp = sp*lineLen;

            if (types[sp])
            {
                tim.setFromTime(times[sp]);

                out.writeString(tim.cstr());
                out.writeString(" <b>[");
                out.writeString(getTypeStr(types[sp]));
                out.writeString("]</b> ");
            }
            str.set(&buf[bp]);
            str.convertTo(String::T_HTML);

            out.writeString(str.cstr());
            out.writeString("<br>");

            sp++;
            sp %= maxLines;
        }
    }

    lb.off();

}

// ---------------------------
void    ThreadInfo::shutdown()
{
    active = false;
    //sys->waitThread(this);
}

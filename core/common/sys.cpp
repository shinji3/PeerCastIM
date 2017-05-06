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
#define isSJIS(a,b) ((a >= 0x81 && a <= 0x9f || a >= 0xe0 && a<=0xfc) && (b >= 0x40 && b <= 0x7e || b >= 0x80 && b<=0xfc))
#define isEUC(a) (a >= 0xa1 && a <= 0xfe)
#define isASCII(a) (a <= 0x7f) 
#define isPLAINASCII(a) (((a >= '0') && (a <= '9')) || ((a >= 'a') && (a <= 'z')) || ((a >= 'A') && (a <= 'Z')))
#define isUTF8(a,b) ((a & 0xc0) == 0xc0 && (b & 0x80) == 0x80 )
#define isESCAPE(a,b) ((a == '&') && (b == '#'))
#define isHTMLSPECIAL(a) ((a == '&') || (a == '\"') || (a == '\'') || (a == '<') || (a == '>'))



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
static int base64chartoval(char input)
{
    if(input >= 'A' && input <= 'Z')
        return input - 'A';
    else if(input >= 'a' && input <= 'z')
        return input - 'a' + 26;
    else if(input >= '0' && input <= '9')
        return input - '0' + 52;
    else if(input == '+')
        return 62;
    else if(input == '/')
        return 63;
    else if(input == '=')
        return -1;
    else
        return -2;
}

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
bool String::isValidURL()
{
    return (strnicmp(data,"http://",7)==0) || (strnicmp(data,"mailto:",7)==0);
}

// -----------------------------------
void String::setFromTime(unsigned int t)
{
//    char *p = ctime((time_t*)&t);
    time_t tmp = t;
    char *p = ctime(&tmp);
    if (p)
        strcpy(data,p);
    else
        strcpy(data,"-");
    type = T_ASCII;
}
// -----------------------------------
void String::setFromStopwatch(unsigned int t)
{
    unsigned int sec,min,hour,day;

    sec = t%60;
    min = (t/60)%60;
    hour = (t/3600)%24;
    day = (t/86400);

    if (day)
        sprintf(data,"%d day, %d hour",day,hour);
    else if (hour)
        sprintf(data,"%d hour, %d min",hour,min);
    else if (min)
        sprintf(data,"%d min, %d sec",min,sec);
    else if (sec)
        sprintf(data,"%d sec",sec);
    else
        sprintf(data,"-");

    type = T_ASCII;
}
// -----------------------------------
void String::setFromString(const char *str, TYPE t)
{
    int cnt=0;
    bool quote=false;
    while (*str)
    {
        bool add=true;
        if (*str == '\"')
        {
            if (quote) 
                break;
            else 
                quote = true;
            add = false;
        }else if (*str == ' ')
        {
            if (!quote)
            {
                if (cnt)
                    break;
                else
                    add = false;
            }
        }

        if (add)
        {
            data[cnt++] = *str++;
            if (cnt >= (MAX_LEN-1))
                break;
        }else
            str++;
    }
    data[cnt] = 0;
    type = t;
}

// -----------------------------------
int String::base64WordToChars(char *out,const char *input)
{
    char *start = out;
    signed char vals[4];

    vals[0] = base64chartoval(*input++);
    vals[1] = base64chartoval(*input++);
    vals[2] = base64chartoval(*input++);
    vals[3] = base64chartoval(*input++);

    if(vals[0] < 0 || vals[1] < 0 || vals[2] < -1 || vals[3] < -1) 
        return 0;

    *out++ = vals[0]<<2 | vals[1]>>4;
    if(vals[2] >= 0)
        *out++ = ((vals[1]&0x0F)<<4) | (vals[2]>>2);
    else
        *out++ = 0;

    if(vals[3] >= 0)
        *out++ = ((vals[2]&0x03)<<6) | (vals[3]);
    else
        *out++ = 0;

    return (int)(out-start);
}

// -----------------------------------
void String::BASE642ASCII(const char *input)
{
    char *out = data;
    int len = (int)strlen(input);

    while(len >= 4) 
    {
        out += base64WordToChars(out,input);
        input += 4;
        len -= 4;
    }
    *out = 0;
}



// -----------------------------------
void String::UNKNOWN2UNICODE(const char *in,bool safe)
{
    MemoryStream utf8(data,MAX_LEN-1);

    unsigned char c;
    unsigned char d;

    while (c = *in++)
    {
        d = *in;

        if (isUTF8(c,d))        // utf8 encoded 
        {
            int numChars=0;
            int i;

            for(i=0; i<6; i++)
            {
                if (c & (0x80>>i))
                    numChars++;
                else
                    break;
            }

            utf8.writeChar(c);
            for(i=0; i<numChars-1; i++)
                utf8.writeChar(*in++);

        }
        else if(isSJIS(c,d))            // shift_jis
        {
            utf8.writeUTF8(JISConverter::sjisToUnicode((c<<8 | d)));
            in++;

        }
        else if(isEUC(c) && isEUC(d))        // euc-jp
        {
            utf8.writeUTF8(JISConverter::eucToUnicode((c<<8 | d)));
            in++;

        }
        else if (isESCAPE(c,d))        // html escape tags &#xx;
        {
            in++;
            char code[16];
            char *cp = code;
            while (c=*in++) 
            {
                if (c!=';')
                    *cp++ = c;
                else
                    break;
            }
            *cp = 0;

            utf8.writeUTF8(strtoul(code,NULL,10));

        }
        else if (isPLAINASCII(c))    // plain ascii : a-z 0-9 etc..
        {
            utf8.writeUTF8(c);

        }
        else if (isHTMLSPECIAL(c) && safe)            
        {
            const char *str = NULL;
            if (c == '&') str = "&amp;";
            else if (c == '\"') str = "&quot;";
            else if (c == '\'') str = "&#039;";
            else if (c == '<') str = "&lt;";
            else if (c == '>') str = "&gt;";
            else str = "?";

            utf8.writeString(str);
        }
        else
        {
            utf8.writeUTF8(c);
        }

        if (utf8.pos >= (MAX_LEN-10))
            break;


    }

    utf8.writeChar(0);    // null terminate

}

// -----------------------------------
void String::ASCII2HTML(const char *in)
{
    char *op = data;
    char *oe = data+MAX_LEN-10;
    unsigned char c;
    const char *p = in;
    while (c = *p++)
    {
        
        if (((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')))
        {
            *op++ = c;
        }else
        {
            sprintf(op,"&#x%02X;",(int)c);
            op+=6;
        }
        if (op >= oe)
            break;
    }
    *op = 0;
}
// -----------------------------------
void String::ASCII2ESC(const char *in, bool safe)
{
    char *op = data;
    char *oe = data+MAX_LEN-10;
    const char *p = in;
    unsigned char c;
    while (c = *p++)
    {
        if (((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')))
            *op++ = c;
        else
        {
            *op++ = '%';
            if (safe)
                *op++ = '%';
            *op=0;
            sprintf(op,"%02X",(int)c);
            op+=2;
        }
        if (op >= oe)
            break;
    }
    *op=0;
}
// -----------------------------------
void String::HTML2ASCII(const char *in)
{
    unsigned char c;
    char *o = data;
    char *oe = data+MAX_LEN-10;
    const char *p = in;
    while (c = *p++)
    {
        if ((c == '&') && (p[0] == '#'))
        {
            p++;
            char code[8];
            char *cp = code;
            char ec = *p++;        // hex/dec
            while (c=*p++) 
            {
                if (c!=';')
                    *cp++ = c;
                else
                    break;
            }
            *cp = 0;
            c = (unsigned char)strtoul(code,NULL,ec=='x'?16:10);
        }
        *o++ = c;
        if (o >= oe)
            break;
    }

    *o=0;
}
// -----------------------------------
void String::HTML2UNICODE(const char *in)
{
    MemoryStream utf8(data,MAX_LEN-1);

    unsigned char c;
    while (c = *in++)
    {
        if ((c == '&') && (*in == '#'))
        {
            in++;
            char code[16];
            char *cp = code;
            char ec = *in++;        // hex/dec
            while (c=*in++) 
            {
                if (c!=';')
                    *cp++ = c;
                else
                    break;
            }
            *cp = 0;
            utf8.writeUTF8(strtoul(code,NULL,ec=='x'?16:10));
        }else
            utf8.writeUTF8(c);

        if (utf8.pos >= (MAX_LEN-10))
            break;
    }

    utf8.writeUTF8(0);
}

// -----------------------------------
void String::ESC2ASCII(const char *in)
{
    unsigned char c;
    char *o = data;
    char *oe = data+MAX_LEN-10;
    const char *p = in;
    while (c = *p++)
    {
        if (c == '+')
            c = ' ';
        else if (c == '%')
        {
            if (p[0] == '%')
                p++;

            char hi = TOUPPER(p[0]);
            char lo = TOUPPER(p[1]);
            c = (TONIBBLE(hi)<<4) | TONIBBLE(lo);
            p+=2;
        }
        *o++ = c;
        if (o >= oe)
            break;
    }

    *o=0;
}
// -----------------------------------
void String::ASCII2META(const char *in, bool safe)
{
    char *op = data;
    char *oe = data+MAX_LEN-10;
    const char *p = in;
    unsigned char c;
    while (c = *p++)
    {
        switch (c)
        {
            case '%':
                if (safe)
                    *op++='%';
                break;
            case ';':
                c = ':';
                break;
        }

        *op++=c;
        if (op >= oe)
            break;
    }
    *op=0;
}
#ifdef WIN32
// -----------------------------------
void String::ASCII2SJIS(const char *in) //JP-EX
{
    char *op = data;
    char *p;
    if (utf8_decode(in,&p)<0)
    {
        strcpy(op,in);
        return;
    }
    strcpy(op,p);
    free(p);
}
#endif
// -----------------------------------
void String::convertTo(TYPE t)
{
    if (t != type)
    {
        String tmp = *this;

        // convert to ASCII
        switch (type)
        {
            case T_UNKNOWN:
            case T_ASCII:
                break;
            case T_HTML:
                tmp.HTML2ASCII(data);
                break;
            case T_ESC:
            case T_ESCSAFE:
                tmp.ESC2ASCII(data);
                break;
            case T_META:
            case T_METASAFE:
                break;
            case T_BASE64:
                tmp.BASE642ASCII(data);
                break;
        }

        // convert to new format
        switch (t)
        {
            case T_UNKNOWN:
            case T_ASCII:
                strcpy(data,tmp.data);
                break;
            case T_UNICODE:
                UNKNOWN2UNICODE(tmp.data,false);
                break;
            case T_UNICODESAFE:
                UNKNOWN2UNICODE(tmp.data,true);
                break;
            case T_HTML:
                ASCII2HTML(tmp.data);
                break;
            case T_ESC:
                ASCII2ESC(tmp.data,false);
                break;
            case T_ESCSAFE:
                ASCII2ESC(tmp.data,true);
                break;
            case T_META:
                ASCII2META(tmp.data,false);
                break;
            case T_METASAFE:
                ASCII2META(tmp.data,true);
                break;
#ifdef WIN32
            case T_SJIS: //JP-EX
                ASCII2SJIS(tmp.data);
                break;
#endif
        }

        type = t;
    }
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

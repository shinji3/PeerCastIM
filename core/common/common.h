// ------------------------------------------------
// File : common.h
// Date: 4-apr-2002
// Author: giles
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

#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <string.h>

#include <string>

#include "gnuid.h"
#include "host.h"

// ----------------------------------
class GeneralException
{
public:
    GeneralException(const char *m, int e = 0)
    {
        strcpy_s(msg, sizeof(msg), m);
        err=e;
    }
    char msg[128];
    int err;
};

// -------------------------------------
class StreamException : public GeneralException
{
public:
    StreamException(const char *m) : GeneralException(m) {}
    StreamException(const char *m, int e) : GeneralException(m, e) {}
};

// ----------------------------------
class SockException : public StreamException
{
public:
    SockException(const char *m="Socket") : StreamException(m) {}
    SockException(const char *m, int e) : StreamException(m, e) {}
};

// ----------------------------------
class EOFException : public StreamException
{
public:
    EOFException(const char *m="EOF") : StreamException(m) {}
    EOFException(const char *m, int e) : StreamException(m, e) {}
};

// ----------------------------------
class CryptException : public StreamException
{
public:
    CryptException(const char *m="Crypt") : StreamException(m) {}
    CryptException(const char *m, int e) : StreamException(m, e) {}
};

// ----------------------------------
class TimeoutException : public StreamException
{
public:
    TimeoutException(const char *m="Timeout") : StreamException(m) {}
};

// ----------------------------------
#define SWAP2(v) ( ((v&0xff)<<8) | ((v&0xff00)>>8) )
#define SWAP3(v) (((v&0xff)<<16) | ((v&0xff00)) | ((v&0xff0000)>>16) )
#define SWAP4(v) (((v&0xff)<<24) | ((v&0xff00)<<8) | ((v&0xff0000)>>8) | ((v&0xff000000)>>24))
#define TOUPPER(c) ((((c) >= 'a') && ((c) <= 'z')) ? (c)+'A'-'a' : (c))
#define TONIBBLE(c) ((((c) >= 'A')&&((c) <= 'F')) ? (((c)-'A')+10) : ((c)-'0'))
#define BYTES_TO_KBPS(n) (float)(((((float)n)*8.0f)/1024.0f))

// -----------------------------------
const char  *getCGIarg(const char *str, const char *arg);
bool        cmpCGIarg(const char *str, const char *arg, const char *value);
bool        hasCGIarg(const char *str, const char *arg);

// ----------------------------------
extern void LOG(const char *fmt, ...);
extern void LOG_ERROR(const char *fmt, ...);
extern void LOG_DEBUG(const char *fmt, ...);
extern void LOG_NETWORK(const char *fmt, ...);
extern void LOG_CHANNEL(const char *fmt, ...);

#endif


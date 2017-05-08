// ------------------------------------------------
// File : gnuid.h
// Author: giles
//
// (c) peercast.org
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

#ifndef _GNUID_H
#define _GNUID_H

#include "common.h"

// --------------------------------
class GnuID
{
public:
    bool    isSame(const GnuID &gid) const
    {
        for(int i=0; i<16; i++)
            if (gid.id[i] != id[i])
                return false;
        return true;
    }


    bool    isSet() const
    {
        for (int i=0; i<16; i++)
            if (id[i] != 0)
                return true;
        return false;
    }

    void    clear()
    {
        for (int i=0; i<16; i++)
            id[i] = 0;
        storeTime = 0;
    }

    void    generate(unsigned char = 0);
    void    encode(class Host *, const char *, const char *, unsigned char);

    void    toStr(char *) const;
    void    fromStr(const char *);

    unsigned char   getFlags();

    unsigned char id[16];
    unsigned int storeTime;
};

// --------------------------------
class GnuIDList
{
public:
    GnuIDList(int);
    ~GnuIDList();

    void            clear();
    void            add(GnuID &);
    bool            contains(GnuID &);
    int             numUsed();
    unsigned int    getOldest();

    GnuID   *ids;
    int     maxID;
};

#endif

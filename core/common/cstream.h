// ------------------------------------------------
// File : cstream.h
// Date: 12-mar-2004
// Author: giles
//
// (c) 2002-4 peercast.org
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

#ifndef _CSTREAM_H
#define _CSTREAM_H

#include <vector>

#include "atom.h"
#include "sys.h"

// ----------------------------------

class Channel;
class ChanPacket;
class ChanPacketv;
class Stream;

// ----------------------------------
class ChanPacket
{
public:
    enum
    {
        MAX_DATALEN = 16384
    };

    enum TYPE
    {
        T_UNKNOWN   = 0,
        T_HEAD      = 1,
        T_DATA      = 2,
        T_META      = 4,
        T_PCP       = 16,
        T_ALL       = 0xff
    };

    ChanPacket()
    {
        init();
    }

    void    init()
    {
        type = T_UNKNOWN;
        len  = 0;
        pos  = 0;
        sync = 0;
        cont = false;
        skip = false;
        priority = 0;
    }

    void    init(ChanPacketv &p);
    void    init(TYPE type, const void *data, unsigned int length, unsigned int position);

    void    writeRaw(Stream &);
    void    writePeercast(Stream &);
    void    readPeercast(Stream &);

    ChanPacket& operator=(const ChanPacket& other);

    TYPE            type;
    unsigned int    len;
    unsigned int    pos;
    unsigned int    sync;
    bool            cont; // true if this is a continuation packet
    bool            skip;
    int             priority;
    char            data[MAX_DATALEN];
};

// ----------------------------------
class ChanPacketv
{
public:
    enum {BSIZE = 0x100};
    ChanPacketv() 
    {
        init();
    }
    ~ChanPacketv()
    {
        free();
    }

    void free()
    {
        if (data) {
            delete [] data;
            data = NULL;
            datasize = 0;
        }
    }
    void reset()
    {
        free();
        init();
    }
    void    init()
    {
        type = ChanPacket::T_UNKNOWN;
        len = 0;
        pos = 0;
        sync = 0;
        skip = false;
        data = NULL;
        datasize = 0;
        priority = 0;
    }
    void init(ChanPacket &p)
    {
        if (data && (datasize < p.len || datasize > p.len + BSIZE * 4)) {
            free();
            data = NULL;
            datasize = 0;
        }
        type = p.type;
        len = p.len;
        pos = p.pos;
        sync = p.sync;
        skip = p.skip;
        priority = p.priority;
        if (!data) {
            datasize = (len & ~(BSIZE - 1)) + BSIZE;
            data = new char[datasize];
        }
        memcpy(data, p.data, len);
    }
    void init(ChanPacketv &p)
    {
        ChanPacket tp;
        tp.init(p);
        init(tp);
    }

    void    writeRaw(Stream &);
    void    writePeercast(Stream &);
    void    readPeercast(Stream &);

    unsigned int sync;
    unsigned int pos;
    ChanPacket::TYPE type;
    unsigned int len;
    char *data;
    unsigned int datasize;
    bool skip;

    int priority;
};

// ----------------------------------
class ChanPacketBuffer
{
public:
    enum {
        MAX_PACKETS = 64,
        NUM_SAFEPACKETS = 60
    };

    ChanPacketBuffer()
    {
        init();
    }

    void    init()
    {
        lock.on();
        lastPos = firstPos = safePos = 0;
        readPos = writePos = 0;
        accept = 0;
        lastWriteTime = 0;
        for (int i = 0; i < MAX_PACKETS; i++) packets[i].reset();
        lock.off();

        lastSkipTime = 0;
    }

    int     copyFrom(ChanPacketBuffer &, unsigned in);

    bool    writePacket(ChanPacket &, bool = false);
    void    readPacket(ChanPacket &);
    void    readPacketPri(ChanPacket &);

    bool    willSkip();

    int     numPending() { return writePos - readPos; }

    unsigned int    getFirstDataPos();
    unsigned int    getLatestPos();
    unsigned int    getOldestPos();
    unsigned int    findOldestPos(unsigned int);
    bool            findPacket(unsigned int, ChanPacket &);
    unsigned int    getStreamPos(unsigned int);
    unsigned int    getStreamPosEnd(unsigned int);
    unsigned int    getLastSync();
    unsigned int    getLatestNonContinuationPos();
    unsigned int    getOldestNonContinuationPos();

    struct Stat
    {
        std::vector<unsigned int> packetLengths;
        int continuations;
        int nonContinuations;
    };

    Stat getStatistics()
    {
        if (writePos == 0)
            return { {}, 0, 0 };

        std::vector<unsigned int> lens;
        int cs = 0, ncs = 0;
        for (unsigned int i = firstPos; i <= lastPos; i++)
        {
            lens.push_back(packets[i % MAX_PACKETS].len);
            if (packets[i % MAX_PACKETS].cont)
                cs++;
            else
                ncs++;
        }
        return { lens, cs, ncs };
    }

    ChanPacketv             packets[MAX_PACKETS];
    volatile unsigned int   lastPos, firstPos, safePos;
    volatile unsigned int   readPos, writePos;
    unsigned int            accept;
    unsigned int            lastWriteTime;
    WLock                   lock;

    unsigned int            lastSkipTime;
};

// ----------------------------------
class ChannelStream
{
public:
    ChannelStream()
    : numListeners(0)
    , numRelays(0)
    , isPlaying(false)
    , fwState(0)
    , lastUpdate(0)
    , lastCheckTime(0)
    , lastClapped(0) //JP-MOD
    , parent(NULL)
    {}

    virtual ~ChannelStream() {}

    void updateStatus(Channel *);
    bool getStatus(Channel *, ChanPacket &);

    virtual void kill() {}
    virtual bool sendPacket(ChanPacket &, GnuID &) { return false; }
    virtual void flush(Stream &) {}
    virtual unsigned int flushUb(Stream &, unsigned int) { return 0; }
    virtual void readHeader(Stream &, Channel *) = 0;
    virtual int  readPacket(Stream &, Channel *) = 0;
    virtual void readEnd(Stream &, Channel *) = 0;

    void    readRaw(Stream &, Channel *);

    int             numRelays;
    int             numListeners;
    bool            isPlaying;
    int             fwState;
    unsigned int    lastUpdate;
    unsigned int    lastCheckTime;
    unsigned int    lastClapped; //JP-MOD

    Channel *parent;
};

#endif


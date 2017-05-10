// ------------------------------------------------
// File : chanhit.h
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

#ifndef _CHANHIT_H
#define _CHANHIT_H

#include <functional>

#include "chaninfo.h"
#include "xml.h"
#include "cstream.h"

class AtomStream;
class ChanHitSearch;

// ----------------------------------
class ChanHit
{
public:
    void    init();
    void    initLocal(int numl, int numr, int nums, int uptm, bool, unsigned int, unsigned int, const Host& = Host());
    void    initLocal(int numl, int numr, int nums, int uptm, bool, bool, unsigned int, Channel*, unsigned int, unsigned int);
    void    initLocal_pp(bool isStealth, int numClaps); //JP-MOD
    XML::Node *createXML();

    void    writeAtoms(AtomStream &, GnuID &);
    bool    writeVariable(Stream &, const String &);

    void    pickNearestIP(Host &);

    Host            host;
    Host            rhost[2];
    unsigned int    numListeners, numRelays, numHops;
    int             clap_pp;    //JP-MOD
    unsigned int    time, upTime, lastContact;
    unsigned int    hitID;
    GnuID           sessionID, chanID;
    unsigned int    version;

    bool            firewalled:1, stable:1, tracker:1, recv:1, yp:1, dead:1, direct:1, relay:1, cin:1;
    bool            relayfull:1, chfull:1, ratefull:1;

    int             status;
    int             servent_id;

    unsigned int    oldestPos, newestPos;
    Host            uphost;
    unsigned int    uphostHops;

    unsigned int    versionVP;
    char            versionExPrefix[2];
    unsigned int    versionExNumber;

    std::string versionString();
    std::string str(bool withPort = false);

    ChanHit *next;

    unsigned int    lastSendSeq;
};

// ----------------------------------
class ChanHitList
{
public:
    ChanHitList();
    ~ChanHitList();

    int          contactTrackers(bool, int, int, int);

    ChanHit      *addHit(ChanHit &);
    void         delHit(ChanHit &);
    void         deadHit(ChanHit &);
    void         clearHits(bool);
    int          numHits();
    int          numListeners();
    int          numClaps();    //JP-MOD
    int          numRelays();
    int          numFirewalled();
    int          numTrackers();
    int          closestHit();
    int          furthestHit();
    unsigned int newestHit();

    int          pickHits(ChanHitSearch &);
    int          pickSourceHits(ChanHitSearch &);

    bool         isUsed() { return used; }
    int          clearDeadHits(unsigned int, bool);
    XML::Node    *createXML(bool addHits = true);

    ChanHit      *deleteHit(ChanHit *);

    int          getTotalListeners();
    int          getTotalRelays();
    int          getTotalFirewalled();

    unsigned int getSeq();

    bool         used;
    ChanInfo     info;
    ChanHit      *hit;
    unsigned int lastHitTime;
    ChanHitList  *next;

    WLock        seqLock;
    unsigned int riSequence;
};

// ----------------------------------
class ChanHitSearch
{
public:
    enum
    {
        MAX_RESULTS = 8
    };

    ChanHitSearch() { init(); }
    void init();

    ChanHit         best[MAX_RESULTS];
    Host            matchHost;
    unsigned int    waitDelay;
    bool            useFirewalled;
    bool            trackersOnly;
    bool            useBusyRelays, useBusyControls;
    GnuID           excludeID;
    int             numResults;
    unsigned int    seed;

    int getRelayHost(Host host1, Host host2, GnuID exID, ChanHitList *chl);
};

#endif

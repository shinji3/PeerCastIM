// ------------------------------------------------
// File : channel.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      Channel streaming classes. These do the actual
//      streaming of media between clients.
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

#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "socket.h"
#include "channel.h"
#include "gnutella.h"
#include "servent.h"
#include "servmgr.h"
#include "sys.h"
#include "xml.h"
#include "http.h"
#include "peercast.h"
#include "atom.h"
#include "pcp.h"

#include "mp3.h"
#include "ogg.h"
#include "mms.h"
#include "nsv.h"
#include "flv.h"

#include "icy.h"
#include "url.h"

#include "version2.h"
#include "win32/seh.h"

#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif

// -----------------------------------
const char *Channel::srcTypes[] =
{
    "NONE",
    "PEERCAST",
    "SHOUTCAST",
    "ICECAST",
    "URL"
};

// -----------------------------------
const char *Channel::statusMsgs[] =
{
    "NONE",
    "WAIT",
    "CONNECT",
    "REQUEST",
    "CLOSE",
    "RECEIVE",
    "BROADCAST",
    "ABORT",
    "SEARCH",
    "NOHOSTS",
    "IDLE",
    "ERROR",
    "NOTFOUND"
};


// for PCRaw start.
bool isIndexTxt(ChanInfo *info)
{
    size_t len;

    if(    info &&
        info->contentType == ChanInfo::T_RAW &&
        info->bitrate <= 32 &&
        (len = strlen(info->name.cstr())) >= 9 &&
        !memcmp(info->name.cstr(), "index", 5) &&
        !memcmp(info->name.cstr()+len-4, ".txt", 4))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool isIndexTxt(Channel *ch)
{
    if(ch && !ch->isBroadcasting() && isIndexTxt(&ch->info))
        return true;
    else
        return false;
}

int numMaxRelaysIndexTxt(Channel *ch)
{
    return ((servMgr->maxRelaysIndexTxt < 1) ? 1 : servMgr->maxRelaysIndexTxt);
}

int canStreamIndexTxt(Channel *ch)
{
    int ret;

    // 自分が配信している場合は関係ない
    if(!ch || ch->isBroadcasting())
        return -1;

    ret = numMaxRelaysIndexTxt(ch) - ch->localRelays();
    if(ret < 0)
        ret = 0;

    return ret;
}
// for PCRaw end.

int channel_count=1;
// -----------------------------------------------------------------------------
// Initialise the channel to its default settings of unallocated and reset.
// -----------------------------------------------------------------------------
Channel::Channel() : maxRelays(0)
{
    next = NULL;
    reset();
    channel_id = channel_count++;
}

// -----------------------------------------------------------------------------
void Channel::endThread(bool flg)
{
    if (pushSock)
    {
        pushSock->close();
        delete pushSock;
        pushSock = NULL;
    }

    if (sock)
    {
        sock->close();
        sock = NULL;
    }

    if (sourceData)
    {
        delete sourceData;
        sourceData = NULL;
    }

    if (flg == true){
        reset();

        chanMgr->channellock.on();
        chanMgr->deleteChannel(this);
        chanMgr->channellock.off();

        sys->endThread(&thread);
    }
}

// -----------------------------------------------------------------------------
void Channel::resetPlayTime()
{
    info.lastPlayStart = sys->getTime();
}

// -----------------------------------------------------------------------------
void Channel::setStatus(STATUS s)
{
    if (s != status)
    {
        bool wasPlaying = isPlaying();

        status = s;

        if (isPlaying())
        {
            info.status = ChanInfo::S_PLAY;
            resetPlayTime();
        }else
        {
            if (wasPlaying)
                info.lastPlayEnd = sys->getTime();
            info.status = ChanInfo::S_UNKNOWN;
        }

        if (isBroadcasting())
        {
            ChanHitList *chl = chanMgr->findHitListByID(info.id);
            if (!chl)
                chanMgr->addHitList(info);
        }

        peercastApp->channelUpdate(&info);
    }
}

// -----------------------------------------------------------------------------
// Reset channel and make it available
// -----------------------------------------------------------------------------
void Channel::reset()
{
    sourceHost.init();
    remoteID.clear();

    streamIndex = 0;

    lastIdleTime = 0;

    info.init();

    mount.clear();
    bump = false;
    stayConnected = false;
    stealth = false; //JP-MOD
    overrideMaxRelaysPerChannel = -1; //JP-MOD
    bClap = false; //JP-MOD

    icyMetaInterval = 0;
    streamPos = 0;
    skipCount = 0; //JP-EX
    lastSkipTime = 0;

    insertMeta.init();

    headPack.init();

    sourceStream = NULL;

    rawData.init();
    rawData.accept = ChanPacket::T_HEAD | ChanPacket::T_DATA;

    setStatus(S_NONE);
    type = T_NONE;

    readDelay = false;
    sock = NULL;
    pushSock = NULL;

    sourceURL.clear();
    sourceData = NULL;

    lastTrackerUpdate = 0;
    lastMetaUpdate = 0;

    srcType = SRC_NONE;

    startTime = 0;
    syncTime = 0;

    channel_id = 0;
    finthread = NULL;

    trackerHit.init();
}

// -----------------------------------
void    Channel::newPacket(ChanPacket &pack)
{
    if (pack.type == ChanPacket::T_PCP)
        return;

    rawData.writePacket(pack, true);
}

// -----------------------------------
bool    Channel::checkIdle()
{
    return
        (info.getUptime() > chanMgr->prefetchTime) &&
        (localListeners() == 0) &&
        (!stayConnected) &&
        (status != S_BROADCASTING);
}

// -----------------------------------
bool    Channel::isFull()
{
    // for PCRaw (relay) start.
    if(isIndexTxt(this))
    {
        int ret = canStreamIndexTxt(this);
        
        if(ret > 0)
            return false;
        else if(ret == 0)
            return true;
    }
    // for PCRaw (relay) end.

    // チャンネル固有のリレー上限設定があるか
    if (maxRelays > 0)
    {
        return localRelays() >= maxRelays;
    } else
    {
        return chanMgr->maxRelaysPerChannel ? localRelays() >= chanMgr->maxRelaysPerChannel : false;
    }
}

// -----------------------------------
int Channel::localRelays()
{
    return servMgr->numStreams(info.id, Servent::T_RELAY, true);
}

// -----------------------------------
int Channel::localListeners()
{
    return servMgr->numStreams(info.id, Servent::T_DIRECT, true);
}

// -----------------------------------
int Channel::totalRelays()
{
    int tot = 0;
    ChanHitList *chl = chanMgr->findHitListByID(info.id);
    if (chl)
        tot += chl->numHits();
    return tot;
}

// -----------------------------------
int Channel::totalListeners()
{
    int tot = localListeners();
    ChanHitList *chl = chanMgr->findHitListByID(info.id);
    if (chl)
        tot += chl->numListeners();
    return tot;
}

// -----------------------------------
int Channel::totalClaps()    //JP-MOD
{
    ChanHitList *chl = chanMgr->findHitListByID(info.id);
    return chl ? chl->numClaps() : 0;
}

// -----------------------------------
void    Channel::startGet()
{
    srcType = SRC_PEERCAST;
    type = T_RELAY;
    info.srcProtocol = ChanInfo::SP_PCP;

    sourceData = new PeercastSource();

    startStream();
}

// -----------------------------------
void    Channel::startURL(const char *u)
{
    sourceURL.set(u);

    srcType = SRC_URL;
    type = T_BROADCAST;
    stayConnected = true;

    resetPlayTime();

    sourceData = new URLSource(u);

    startStream();
}

// -----------------------------------
void Channel::startStream()
{
    thread.data = this;
    thread.func = stream;
    if (!sys->startThread(&thread))
        reset();
}

// -----------------------------------
void Channel::sleepUntil(double time)
{
    double sleepTime = time - (sys->getDTime()-startTime);

//  LOG("sleep %g", sleepTime);
    if (sleepTime > 0)
    {
        if (sleepTime > 60) sleepTime = 60;

        double sleepMS = sleepTime*1000;

        sys->sleep((int)sleepMS);
    }
}

// -----------------------------------
void Channel::checkReadDelay(unsigned int len)
{
    if (readDelay)
    {
        unsigned int time = (len*1000)/((info.bitrate*1024)/8);
        sys->sleep(time);
    }
}

// -----------------------------------
THREAD_PROC Channel::streamMain(ThreadInfo *thread)
{
//    thread->lock();

    Channel *ch = (Channel *)thread->data;

    LOG_CHANNEL("Channel started");

    while (thread->active && !peercastInst->isQuitting && !thread->finish)
    {
        ChanHitList *chl = chanMgr->findHitList(ch->info);
        if (!chl)
            chanMgr->addHitList(ch->info);

        ch->sourceData->stream(ch);

        LOG_CHANNEL("Channel stopped");
        ch->rawData.init();

        if (!ch->stayConnected)
        {
            thread->active = false;
            break;
        }else
        {
            if (!ch->info.lastPlayEnd)
                ch->info.lastPlayEnd = sys->getTime();

            unsigned int diff = (sys->getTime() - ch->info.lastPlayEnd) + 5;

            LOG_DEBUG("Channel sleeping for %d seconds", diff);
            for (unsigned int i=0; i<diff; i++)
            {
                if (ch->info.lastPlayEnd == 0) // reconnected
                    break;
                if (!thread->active || peercastInst->isQuitting){
                    thread->active = false;
                    break;
                }
                sys->sleep(1000);
            }
        }
    }

    LOG_DEBUG("thread.active = %d, thread.finish = %d",
        ch->thread.active, ch->thread.finish);

    if (!thread->finish){
        ch->endThread(false);

        if (!ch->finthread){
            ch->finthread = new ThreadInfo();
            ch->finthread->func = waitFinish;
            ch->finthread->data = ch;
            sys->startThread(ch->finthread);
        }
    } else {
        ch->endThread(true);
    }
    return 0;
}

// -----------------------------------
THREAD_PROC    Channel::stream(ThreadInfo *thread)
{
    SEH_THREAD(streamMain, Channel::stream);
}    

// -----------------------------------
THREAD_PROC Channel::waitFinishMain(ThreadInfo *thread)
{
    Channel *ch = (Channel*)thread->data;
    LOG_DEBUG("Wait channel finish");

    while(!(ch->thread.finish) && !thread->finish){
        sys->sleep(1000);
    }

    if (ch->thread.finish){
        LOG_DEBUG("channel finish");
        ch->endThread(true);
    } else {
        LOG_DEBUG("channel restart");
    }

    delete thread;
    return 0;
}

// -----------------------------------
THREAD_PROC Channel::waitFinish(ThreadInfo *thread)
{
    SEH_THREAD(waitFinishMain, Channel::waitFinish);
}


// -----------------------------------
bool Channel::acceptGIV(ClientSocket *givSock)
{
    if (!pushSock)
    {
        pushSock = givSock;
        return true;
    }else
        return false;
}

// -----------------------------------
void Channel::connectFetch()
{
    sock = sys->createSocket();

    if (!sock)
        throw StreamException("Can`t create socket");

    if (sourceHost.tracker || sourceHost.yp)
    {
        sock->setReadTimeout(30000);
        sock->setWriteTimeout(30000);
        LOG_CHANNEL("Channel using longer timeouts");
    } else {
        sock->setReadTimeout(5000);
        sock->setWriteTimeout(5000);
    }

    sock->open(sourceHost.host);

    sock->connect();
}

// -----------------------------------
int Channel::handshakeFetch()
{
    char idStr[64];
    info.id.toStr(idStr);

    char sidStr[64];
    servMgr->sessionID.toStr(sidStr);

    sock->writeLineF("GET /channel/%s HTTP/1.0", idStr);
    sock->writeLineF("%s %d", PCX_HS_POS, streamPos);
    sock->writeLineF("%s %d", PCX_HS_PCP, 1);
    sock->writeLineF("%s %d", PCX_HS_PORT, servMgr->serverHost.port);

    sock->writeLine("");

    HTTP http(*sock);

    int r = http.readResponse();

    LOG_CHANNEL("Got response: %d", r);

    while (http.nextHeader())
    {
        char *arg = http.getArgStr();
        if (!arg)
            continue;

        if (http.isHeader(PCX_HS_POS))
            streamPos = atoi(arg);
        else
            Servent::readICYHeader(http, info, NULL, 0);

        LOG_CHANNEL("Channel fetch: %s", http.cmdLine);
    }

    if ((r != 200) && (r != 503))
        return r;

    if (!servMgr->keepDownstreams) {
        if (rawData.getLatestPos() > streamPos)
            rawData.init();
    }

    AtomStream atom(*sock);

    String agent;

    Host rhost = sock->host;

    if (info.srcProtocol == ChanInfo::SP_PCP)
    {
        // don`t need PCP_CONNECT here
        Servent::handshakeOutgoingPCP(atom, rhost, remoteID, agent, sourceHost.yp|sourceHost.tracker);
    }

    if (r == 503) return 503;
    return 0;
}

// -----------------------------------
void PeercastSource::stream(Channel *ch)
{
    int numYPTries=0;
    int numYPTries2=0;
    bool next_yp = false;
    bool tracker_check = (ch->trackerHit.host.ip != 0);
    int connFailCnt = 0;
    int keepDownstreamTime = 7;

    if (isIndexTxt(&ch->info))
        keepDownstreamTime = 30;

    ch->lastStopTime = 0;
    ch->bumped = false;

    while (ch->thread.active)
    {
        ch->skipCount = 0; //JP-EX
        ch->lastSkipTime = 0;
        
        ChanHitList *chl = NULL;

        ch->sourceHost.init();

        if (connFailCnt >= 3 && (ch->localListeners() == 0) && (!ch->stayConnected) && !ch->isBroadcasting()) {
            ch->lastIdleTime = sys->getTime();
            ch->setStatus(Channel::S_IDLE);
            ch->skipCount = 0;
            ch->lastSkipTime = 0;
            break;
        }

        if (!servMgr->keepDownstreams && !ch->bumped) {
            ch->trackerHit.lastContact = sys->getTime() - 30 + (rand() % 30);
        }

        ch->setStatus(Channel::S_SEARCHING);
        LOG_CHANNEL("Channel searching for hit..");
        do
        {
            if (ch->pushSock)
            {
                ch->sock = ch->pushSock;
                ch->pushSock = NULL;
                ch->sourceHost.host = ch->sock->host;
                break;
            }

            chanMgr->hitlistlock.on();

            chl = chanMgr->findHitList(ch->info);
            if (chl)
            {
                ChanHitSearch chs;

                // find local hit 
                if (!ch->sourceHost.host.ip){
                    chs.init();
                    chs.matchHost = servMgr->serverHost;
                    chs.waitDelay = MIN_RELAY_RETRY;
                    chs.excludeID = servMgr->sessionID;
                    if (chl->pickSourceHits(chs)){
                        ch->sourceHost = chs.best[0];
                        LOG_DEBUG("use local hit");
                    }
                }
                
                // else find global hit
                if (!ch->sourceHost.host.ip)
                {
                    chs.init();
                    chs.waitDelay = MIN_RELAY_RETRY;
                    chs.excludeID = servMgr->sessionID;
                    if (chl->pickSourceHits(chs)){
                        ch->sourceHost = chs.best[0];
                        LOG_DEBUG("use global hit");
                    }
                }

                // else find local tracker
                if (!ch->sourceHost.host.ip)
                {
                    chs.init();
                    chs.matchHost = servMgr->serverHost;
                    chs.waitDelay = MIN_TRACKER_RETRY;
                    chs.excludeID = servMgr->sessionID;
                    chs.trackersOnly = true;
                    if (chl->pickSourceHits(chs)){
                        ch->sourceHost = chs.best[0];
                        LOG_DEBUG("use local tracker");
                    }
                }

                // else find global tracker
                if (!ch->sourceHost.host.ip)
                {
                    chs.init();
                    chs.waitDelay = MIN_TRACKER_RETRY;
                    chs.excludeID = servMgr->sessionID;
                    chs.trackersOnly = true;
                    if (chl->pickSourceHits(chs)){
                        ch->sourceHost = chs.best[0];
                        tracker_check = true;
                        ch->trackerHit = chs.best[0];
                        LOG_DEBUG("use global tracker");
                    }
                }

                // find tracker
                unsigned int ctime = sys->getTime();
                if (!ch->sourceHost.host.ip && tracker_check && ch->trackerHit.host.ip){
                    if (ch->trackerHit.lastContact + 30 < ctime){
                        ch->sourceHost = ch->trackerHit;
                        ch->trackerHit.lastContact = ctime;
                        LOG_DEBUG("use saved tracker");
                    }
                }
            }

            chanMgr->hitlistlock.off();

            if (servMgr->keepDownstreams && ch->lastStopTime
                && ch->lastStopTime < sys->getTime() - keepDownstreamTime)
            {
                ch->lastStopTime = 0;
                LOG_DEBUG("------------ disconnect all downstreams");
                ChanPacket pack;
                MemoryStream mem(pack.data,sizeof(pack.data));
                AtomStream atom(mem);
                atom.writeInt(PCP_QUIT,PCP_ERROR_QUIT+PCP_ERROR_OFFAIR);
                pack.len = mem.pos;
                pack.type = ChanPacket::T_PCP;
                GnuID noID;
                noID.clear();
                servMgr->broadcastPacket(pack,ch->info.id,ch->remoteID,noID,Servent::T_RELAY);

                chanMgr->hitlistlock.on();
                ChanHitList *hl = chanMgr->findHitList(ch->info);
                if (hl){
                    hl->clearHits(false);
                }
                chanMgr->hitlistlock.off();
            }

            // no trackers found so contact YP
            if (!tracker_check && !ch->sourceHost.host.ip)
            {
                next_yp = false;
                if (servMgr->rootHost.isEmpty())
                    goto yp2;

                if (numYPTries >= 3)
                    goto yp2;

                if  ((!servMgr->rootHost2.isEmpty()) && (numYPTries > numYPTries2))
                    goto yp2;

                unsigned int ctime=sys->getTime();
                if ((ctime-chanMgr->lastYPConnect) > MIN_YP_RETRY)
                {
                    ch->sourceHost.host.fromStrName(servMgr->rootHost.cstr(),DEFAULT_PORT);
                    ch->sourceHost.yp = true;
                    chanMgr->lastYPConnect=ctime;
                    numYPTries++;
                }
                if (numYPTries < 3)
                    next_yp = true;
            }

yp2:
            // no trackers found so contact YP2
            if (!tracker_check && !ch->sourceHost.host.ip)
            {
//                next_yp = false;
                if (servMgr->rootHost2.isEmpty())
                    goto yp0;

                if (numYPTries2 >= 3)
                    goto yp0;

                unsigned int ctime=sys->getTime();
                if ((ctime-chanMgr->lastYPConnect2) > MIN_YP_RETRY)
                {
                    ch->sourceHost.host.fromStrName(servMgr->rootHost2.cstr(),DEFAULT_PORT);
                    ch->sourceHost.yp = true;
                    chanMgr->lastYPConnect2=ctime;
                    numYPTries2++;
                }
                if (numYPTries2 < 3)
                    next_yp = true;
            }
yp0:
            if (!tracker_check && !ch->sourceHost.host.ip && !next_yp) break;

            sys->sleepIdle();

        }while((ch->sourceHost.host.ip==0) && (ch->thread.active));

        if (!ch->sourceHost.host.ip)
        {
            LOG_ERROR("Channel giving up");
            ch->setStatus(Channel::S_ERROR);
            break;
        }

        if (ch->sourceHost.yp)
        {
            LOG_CHANNEL("Channel contacting YP, try %d",numYPTries);
        }else
        {
            LOG_CHANNEL("Channel found hit");
            numYPTries=0;
            numYPTries2=0;
        }

        if (ch->sourceHost.host.ip)
        {
            bool isTrusted = ch->sourceHost.tracker | ch->sourceHost.yp;

            //if (ch->sourceHost.tracker)
            //    peercastApp->notifyMessage(ServMgr::NT_PEERCAST,"Contacting tracker, please wait...");

            char ipstr[64];
            ch->sourceHost.host.toStr(ipstr);

            const char *type = "";
            if (ch->sourceHost.tracker)
                type = "(tracker)";
            else if (ch->sourceHost.yp)
                type = "(YP)";

            int error=-1;
            int got503 = 0;
            try
            {
                ch->setStatus(Channel::S_CONNECTING);
                ch->sourceHost.lastContact = sys->getTime();

                if (!ch->sock)
                {
                    LOG_CHANNEL("Channel connecting to %s %s", ipstr, type);
                    ch->connectFetch();
                }

                error = ch->handshakeFetch();
                if (error == 503) {
                    got503 = 1;
                    error = 0;
                }
                if (error)
                    throw StreamException("Handshake error");
                if (ch->sourceHost.tracker) connFailCnt = 0;

                if (servMgr->autoMaxRelaySetting) //JP-EX
                {    
                    double setMaxRelays = ch->info.bitrate?servMgr->maxBitrateOut/(ch->info.bitrate*1.3):0;
                    if ((unsigned int)setMaxRelays == 0)
                        servMgr->maxRelays = 1;
                    else if ((unsigned int)setMaxRelays > servMgr->autoMaxRelaySetting)
                        servMgr->maxRelays = servMgr->autoMaxRelaySetting;
                    else
                        servMgr->maxRelays = (unsigned int)setMaxRelays;
                }

                ch->sourceStream = ch->createSource();

                error = ch->readStream(*ch->sock, ch->sourceStream);
                if (error)
                    throw StreamException("Stream error");

                error = 0;      // no errors, closing normally.
//              ch->setStatus(Channel::S_CLOSING);            
                ch->setStatus(Channel::S_IDLE);

                LOG_CHANNEL("Channel closed normally");
            }catch (StreamException &e)
            {
                ch->setStatus(Channel::S_ERROR);
                LOG_ERROR("Channel to %s %s : %s", ipstr, type, e.msg);
                if (!servMgr->allowConnectPCST) //JP-EX
                {
                    if (ch->info.srcProtocol == ChanInfo::SP_PEERCAST)
                        ch->thread.active = false;
                }
                //if (!ch->sourceHost.tracker || ((error != 503) && ch->sourceHost.tracker))
                if (!ch->sourceHost.tracker || (!got503 && ch->sourceHost.tracker))
                    chanMgr->deadHit(ch->sourceHost);
                if (ch->sourceHost.tracker && error == -1) {
                    LOG_ERROR("can't connect to tracker");
                    connFailCnt++;
                }
            }

            unsigned int ctime = sys->getTime();
            if (ch->rawData.lastWriteTime) {
                ch->lastStopTime = ch->rawData.lastWriteTime;
                if (isIndexTxt(ch) && ctime - ch->lastStopTime < 60)
                    ch->lastStopTime = ctime;
            }

            if (tracker_check && ch->sourceHost.tracker)
                ch->trackerHit.lastContact = ctime - 30 + (rand() % 30);

            // broadcast source host
            if (!got503 && !error && ch->sourceHost.host.ip) { // if closed normally
                ChanPacket pack;
                MemoryStream mem(pack.data,sizeof(pack.data));
                AtomStream atom(mem);
                ch->sourceHost.writeAtoms(atom, ch->info.id);
                pack.len = mem.pos;
                pack.type = ChanPacket::T_PCP;
                GnuID noID;
                noID.clear();
                servMgr->broadcastPacket(pack,ch->info.id,ch->remoteID,noID,Servent::T_RELAY);
                LOG_DEBUG("stream: broadcast sourceHost");
            }

            // broadcast quit to any connected downstream servents
            if (!servMgr->keepDownstreams || !got503 && (ch->sourceHost.tracker || !error)) {
                ChanPacket pack;
                MemoryStream mem(pack.data, sizeof(pack.data));
                AtomStream atom(mem);
                atom.writeInt(PCP_QUIT, PCP_ERROR_QUIT+PCP_ERROR_OFFAIR);
                pack.len = mem.pos;
                pack.type = ChanPacket::T_PCP;
                GnuID noID;
                noID.clear();
                servMgr->broadcastPacket(pack,ch->info.id,ch->remoteID,noID,Servent::T_RELAY);
                LOG_DEBUG("------------ broadcast quit to all downstreams");

                chanMgr->hitlistlock.on();
                ChanHitList *hl = chanMgr->findHitList(ch->info);
                if (hl){
                    hl->clearHits(false);
                }
                chanMgr->hitlistlock.off();
            }

            if (ch->sourceStream)
            {
                try
                {
                    if (!error)
                    {
                        ch->sourceStream->updateStatus(ch);
                        ch->sourceStream->flush(*ch->sock);
                    }
                }catch (StreamException &)
                {}
                ChannelStream *cs = ch->sourceStream;
                ch->sourceStream = NULL;
                cs->kill();
                delete cs;
            }

            if (ch->sock)
            {
                ch->sock->close();
                delete ch->sock;
                ch->sock = NULL;
            }

            if (error == 404)
            {
                LOG_ERROR("Channel not found");
                //if (!next_yp){
                if ((ch->sourceHost.yp && !next_yp) || ch->sourceHost.tracker) {
                    chanMgr->hitlistlock.on();
                    ChanHitList *hl = chanMgr->findHitList(ch->info);
                    if (hl){
                        hl->clearHits(true);
                    }
                    chanMgr->hitlistlock.off();

                    if(!isIndexTxt(&ch->info))    // for PCRaw (popup)
                        peercastApp->notifyMessage(ServMgr::NT_PEERCAST,"Channel not found");
                    return;
                }
            }
        }

        ch->lastIdleTime = sys->getTime();
        ch->setStatus(Channel::S_IDLE);
        ch->skipCount = 0; //JP-EX
        ch->lastSkipTime = 0;
        while ((ch->checkIdle()) && (ch->thread.active))
        {
            sys->sleepIdle();
        }

        sys->sleepIdle();
    }
}

// -----------------------------------
void    Channel::startICY(ClientSocket *cs, SRC_TYPE st)
{
    srcType = st;
    type = T_BROADCAST;
    cs->setReadTimeout(0);  // stay connected even when theres no data coming through
    sock = cs;
    info.srcProtocol = ChanInfo::SP_HTTP;

    streamIndex = ++chanMgr->icyIndex;

    sourceData = new ICYSource();
    startStream();
}

// -----------------------------------
static char *nextMetaPart(char *str, char delim)
{
    while (*str)
    {
        if (*str == delim)
        {
            *str++ = 0;
            return str;
        }
        str++;
    }
    return NULL;
}

// -----------------------------------
static void copyStr(char *to, char *from, int max)
{
    char c;
    while ((c=*from++) && (--max))
        if (c != '\'')
            *to++ = c;

    *to = 0;
}

// -----------------------------------
void Channel::processMp3Metadata(char *str)
{
    ChanInfo newInfo = info;

    char *cmd=str;
    while (cmd)
    {
        char *arg = nextMetaPart(cmd, '=');
        if (!arg)
            break;

        char *next = nextMetaPart(arg, ';');

        if (strcmp(cmd, "StreamTitle")==0)
        {
            newInfo.track.title.setUnquote(arg, String::T_ASCII);
            newInfo.track.title.convertTo(String::T_UNICODE);
        }else if (strcmp(cmd, "StreamUrl")==0)
        {
            newInfo.track.contact.setUnquote(arg, String::T_ASCII);
            newInfo.track.contact.convertTo(String::T_UNICODE);
        }

        cmd = next;
    }

    updateInfo(newInfo);
}

// -----------------------------------
XML::Node *ChanHit::createXML()
{
    // IP
    char ipStr[64];
    host.toStr(ipStr);

    return new XML::Node("host ip=\"%s\" hops=\"%d\" listeners=\"%d\" relays=\"%d\" uptime=\"%d\" push=\"%d\" relay=\"%d\" direct=\"%d\" cin=\"%d\" stable=\"%d\" version=\"%d\" update=\"%d\" tracker=\"%d\"",
        ipStr,
        numHops,
        numListeners,
        numRelays,
        upTime,
        firewalled?1:0,
        relay?1:0,
        direct?1:0,
        cin?1:0,
        stable?1:0,
        version,
        sys->getTime()-time,
        tracker
        );
}

// -----------------------------------
XML::Node *ChanHitList::createXML(bool addHits)
{
    XML::Node *hn = new XML::Node("hits hosts=\"%d\" listeners=\"%d\" relays=\"%d\" firewalled=\"%d\" closest=\"%d\" furthest=\"%d\" newest=\"%d\"",
        numHits(),
        numListeners(),
        numRelays(),
        numFirewalled(),
        closestHit(),
        furthestHit(),
        sys->getTime()-newestHit()
        );

    if (addHits)
    {
        ChanHit *h = hit;
        while (h)
        {
            if (h->host.ip)
                hn->add(h->createXML());
            h = h->next;
        }
    }

    return hn;
}

// -----------------------------------
XML::Node *Channel::createRelayXML(bool showStat)
{
    const char *ststr;
    ststr = getStatusStr();
    if (!showStat)
        if ((status == S_RECEIVING) || (status == S_BROADCASTING))
            ststr = "OK";

    ChanHitList *chl = chanMgr->findHitList(info);

    return new XML::Node("relay listeners=\"%d\" relays=\"%d\" hosts=\"%d\" status=\"%s\"",
        localListeners(),
        localRelays(),
        (chl!=NULL)?chl->numHits():0,
        ststr
        );
}

// -----------------------------------
void ChanMeta::fromXML(XML &xml)
{
    MemoryStream tout(data, MAX_DATALEN);
    xml.write(tout);

    len = tout.pos;
}

// -----------------------------------
void ChanMeta::fromMem(void *p, int l)
{
    len = l;
    memcpy(data, p, len);
}

// -----------------------------------
void ChanMeta::addMem(void *p, int l)
{
    if ((len+l) <= MAX_DATALEN)
    {
        memcpy(data+len, p, l);
        len += l;
    }
}

// -----------------------------------
void Channel::broadcastTrackerUpdate(GnuID &svID, bool force)
{
    unsigned int ctime = sys->getTime();

    if (((ctime-lastTrackerUpdate) > 30) || (force))
    {
        ChanPacket pack;

        MemoryStream mem(pack.data, sizeof(pack));

        AtomStream atom(mem);

        ChanHit hit;

        ChanHitList *chl = chanMgr->findHitListByID(info.id);
        if (!chl)
            throw StreamException("Broadcast channel has no hitlist");

        int numListeners = stealth ? -1 : totalListeners(); //JP-MOD リスナー数隠蔽機能
        int numRelays = stealth ? -1 : totalRelays(); //JP-MOD リレー数隠蔽機能

        unsigned int oldp = rawData.getOldestPos();
        unsigned int newp = rawData.getLatestPos();

        hit.initLocal(numListeners, numRelays, info.numSkips, info.getUptime(), isPlaying(), false, 0, this, oldp, newp);
        hit.tracker = true;

        if (version_ex == 0)
        {
            atom.writeParent(PCP_BCST,8);
        } else
        {
            atom.writeParent(PCP_BCST,10);
        }
        atom.writeChar(PCP_BCST_GROUP,PCP_BCST_GROUP_ROOT);
        atom.writeChar(PCP_BCST_HOPS,0);
        atom.writeChar(PCP_BCST_TTL,11);
        atom.writeBytes(PCP_BCST_FROM,servMgr->sessionID.id,16);
        atom.writeInt(PCP_BCST_VERSION,PCP_CLIENT_VERSION);
        atom.writeInt(PCP_BCST_VERSION_VP,PCP_CLIENT_VERSION_VP);

        if (version_ex)
        {
            atom.writeBytes(PCP_BCST_VERSION_EX_PREFIX,PCP_CLIENT_VERSION_EX_PREFIX,2);
            atom.writeShort(PCP_BCST_VERSION_EX_NUMBER,PCP_CLIENT_VERSION_EX_NUMBER);
        }
        atom.writeParent(PCP_CHAN,4);
        atom.writeBytes(PCP_CHAN_ID,info.id.id,16);
        atom.writeBytes(PCP_CHAN_BCID,chanMgr->broadcastID.id,16);
        info.writeInfoAtoms(atom);
        info.writeTrackAtoms(atom);
        hit.writeAtoms(atom,info.id);

        pack.len = mem.pos;
        pack.type = ChanPacket::T_PCP;

        GnuID noID;
        noID.clear();
        int cnt = servMgr->broadcastPacket(pack, noID, servMgr->sessionID, svID, Servent::T_COUT);

        if (cnt)
        {
            LOG_DEBUG("Sent tracker update for %s to %d client(s)", info.name.cstr(), cnt);
            lastTrackerUpdate = ctime;
        }
    }
}

// -----------------------------------
bool    Channel::sendPacketUp(ChanPacket &pack, GnuID &cid, GnuID &sid, GnuID &did)
{
    if ( isActive()
        && (!cid.isSet() || info.id.isSame(cid))
        && (!sid.isSet() || !remoteID.isSame(sid))
        && sourceStream
       )
        return sourceStream->sendPacket(pack, did);

    return false;
}

// -----------------------------------
void Channel::updateInfo(const ChanInfo &newInfo)
{
    if (info.update(newInfo))
    {
        if (isBroadcasting())
        {
            unsigned int ctime = sys->getTime();
            if ((ctime-lastMetaUpdate) > 30)
            {
                lastMetaUpdate = ctime;

                ChanPacket pack;

                MemoryStream mem(pack.data,sizeof(pack));

                AtomStream atom(mem);

                if (version_ex == 0)
                {
                    atom.writeParent(PCP_BCST,8);
                } else
                {
                    atom.writeParent(PCP_BCST,10);
                }
                atom.writeChar(PCP_BCST_HOPS,0);
                atom.writeChar(PCP_BCST_TTL,7);
                atom.writeChar(PCP_BCST_GROUP,PCP_BCST_GROUP_RELAYS);
                atom.writeBytes(PCP_BCST_FROM,servMgr->sessionID.id,16);
                atom.writeInt(PCP_BCST_VERSION,PCP_CLIENT_VERSION);
                atom.writeInt(PCP_BCST_VERSION_VP,PCP_CLIENT_VERSION_VP);
                if (version_ex)
                {
                    atom.writeBytes(PCP_BCST_VERSION_EX_PREFIX,PCP_CLIENT_VERSION_EX_PREFIX,2);
                    atom.writeShort(PCP_BCST_VERSION_EX_NUMBER,PCP_CLIENT_VERSION_EX_NUMBER);
                }
                atom.writeBytes(PCP_BCST_CHANID,info.id.id,16);
                atom.writeParent(PCP_CHAN,3);
                atom.writeBytes(PCP_CHAN_ID,info.id.id,16);
                info.writeInfoAtoms(atom);
                info.writeTrackAtoms(atom);

                pack.len = mem.pos;
                pack.type = ChanPacket::T_PCP;
                GnuID noID;
                noID.clear();
                servMgr->broadcastPacket(pack,info.id,servMgr->sessionID,noID,Servent::T_RELAY);

                broadcastTrackerUpdate(noID);
            }
        }

        ChanHitList *chl = chanMgr->findHitList(info);
        if (chl)
            chl->info = info;

        peercastApp->channelUpdate(&info);
    }
}

// -----------------------------------
ChannelStream *Channel::createSource()
{
//  if (servMgr->relayBroadcast)
//      chanMgr->broadcastRelays(NULL, chanMgr->minBroadcastTTL, chanMgr->maxBroadcastTTL);

    ChannelStream *source=NULL;

    if (info.srcProtocol == ChanInfo::SP_PEERCAST)
    {
        LOG_CHANNEL("Channel is Peercast");
        if (servMgr->allowConnectPCST) //JP-EX
            source = new PeercastStream();
        else
            throw StreamException("Channel is not allowed");
    }
    else if (info.srcProtocol == ChanInfo::SP_PCP)
    {
        LOG_CHANNEL("Channel is PCP");
        PCPStream *pcp = new PCPStream(remoteID);
        source = pcp;
    }
    else if (info.srcProtocol == ChanInfo::SP_MMS)
    {
        LOG_CHANNEL("Channel is MMS");
        source = new MMSStream();
    }else
    {
        switch (info.contentType)
        {
            case ChanInfo::T_MP3:
                LOG_CHANNEL("Channel is MP3 - meta: %d", icyMetaInterval);
                source = new MP3Stream();
                break;
            case ChanInfo::T_NSV:
                LOG_CHANNEL("Channel is NSV");
                source = new NSVStream();
                break;
            case ChanInfo::T_WMA:
            case ChanInfo::T_WMV:
                throw StreamException("Channel is WMA/WMV - but not MMS");
                break;
            case ChanInfo::T_FLV:
                LOG_CHANNEL("Channel is FLV");
                source = new FLVStream();
                break;
            case ChanInfo::T_OGG:
            case ChanInfo::T_OGM:
                LOG_CHANNEL("Channel is OGG");
                source = new OGGStream();
                break;
            default:
                LOG_CHANNEL("Channel is Raw");
                source = new RawStream();
                break;
        }
    }

    source->parent = this;

    return source;
}

// -----------------------------------
bool    Channel::checkBump()
{
    unsigned int maxIdleTime = 30;
    if (isIndexTxt(this)) maxIdleTime = 60;

    if (!isBroadcasting() && (!sourceHost.tracker))
        if (rawData.lastWriteTime && ((sys->getTime() - rawData.lastWriteTime) > maxIdleTime))
        {
            LOG_ERROR("Channel Auto bumped");
            bump = true;
        }

    if (bump)
    {
        bump = false;
        return true;
    }else
        return false;
}

// -----------------------------------
int Channel::readStream(Stream &in, ChannelStream *source)
{
    //sys->sleep(300);

    int error = 0;

    info.numSkips = 0;

    source->readHeader(in, this);

    peercastApp->channelStart(&info);

    rawData.lastWriteTime = 0;

    bool wasBroadcasting=false;

    unsigned int receiveStartTime = 0;

    unsigned int ptime = 0;
    unsigned int upsize = 0;

    try
    {
        while (thread.active && !peercastInst->isQuitting)
        {
            if (checkIdle())
            {
                LOG_DEBUG("Channel idle");
                break;
            }

            if (checkBump())
            {
                LOG_DEBUG("Channel bumped");
                error = -1;
                bumped = true;
                break;
            }

            if (in.eof())
            {
                LOG_DEBUG("Channel eof");
                break;
            }

            if (in.readReady())
            {
                error = source->readPacket(in, this);

                if (error)
                    break;

                //if (rawData.writePos > 0)
                if (rawData.lastWriteTime > 0 || rawData.lastSkipTime > 0)
                {
                    if (isBroadcasting())
                    {
                        if ((sys->getTime() - lastTrackerUpdate) >= chanMgr->hostUpdateInterval)
                        {
                            GnuID noID;
                            noID.clear();
                            broadcastTrackerUpdate(noID);
                        }
                        wasBroadcasting = true;
                    }else
                    {
/*                        if (status != Channel::S_RECEIVING){
                            receiveStartTime = sys->getTime();
                        } else if (receiveStartTime && receiveStartTime + 10 > sys->getTime()){
                            chanMgr->hitlistlock.on();
                            ChanHitList *hl = chanMgr->findHitList(info);
                            if (hl){
                                hl->clearHits(true);
                            }
                            chanMgr->hitlistlock.off();
                            receiveStartTime = 0;
                        }*/
                        setStatus(Channel::S_RECEIVING);
                        bumped = false;
                    }
                    //source->updateStatus(this);
                }
            }
            if (rawData.lastWriteTime > 0 || rawData.lastSkipTime > 0)
                source->updateStatus(this);

            unsigned int t = sys->getTime();
            if (t != ptime) {
                ptime = t;
                upsize = Servent::MAX_OUTWARD_SIZE;
            }

            unsigned int len = source->flushUb(in, upsize);
            upsize -= len;

            sys->sleepIdle();
        }
    }catch (StreamException &e)
    {
        LOG_ERROR("readStream: %s", e.msg);
        error = -1;
    }

    if (!servMgr->keepDownstreams) {
        if (status == Channel::S_RECEIVING){
            chanMgr->hitlistlock.on();
            ChanHitList *hl = chanMgr->findHitList(info);
            if (hl){
                hl->clearHits(false);
            }
            chanMgr->hitlistlock.off();
        }
    }

//    setStatus(S_CLOSING);
    setStatus(S_IDLE);

    if (wasBroadcasting)
    {
        GnuID noID;
        noID.clear();
        broadcastTrackerUpdate(noID, true);
    }

    peercastApp->channelStop(&info);

    source->readEnd(in, this);

    return error;
}

// -----------------------------------
void PeercastStream::readHeader(Stream &in, Channel *ch)
{
    if (in.readTag() != 'PCST')
        throw StreamException("Not PeerCast stream");
}

// -----------------------------------
void PeercastStream::readEnd(Stream &, Channel *)
{
}

// -----------------------------------
int PeercastStream::readPacket(Stream &in, Channel *ch)
{
    ChanPacket pack;

    {

        pack.readPeercast(in);

        MemoryStream mem(pack.data,pack.len);

        switch(pack.type)
        {
            case ChanPacket::T_HEAD:
                // update sync pos
                ch->headPack = pack;
                pack.pos = ch->streamPos;
                ch->newPacket(pack);
                ch->streamPos+=pack.len;
                break;
            case ChanPacket::T_DATA:
                pack.pos = ch->streamPos;
                ch->newPacket(pack);
                ch->streamPos+=pack.len;
                break;
            case ChanPacket::T_META:
                ch->insertMeta.fromMem(pack.data,pack.len);
                {
                    if (pack.len)
                    {
                        XML xml;
                        xml.read(mem);
                        XML::Node *n = xml.findNode("channel");                    
                        if (n)
                        {
                            ChanInfo newInfo = ch->info;
                            newInfo.updateFromXML(n);
                            ChanHitList *chl = chanMgr->findHitList(ch->info);
                            if (chl)
                                newInfo.updateFromXML(n);
                            ch->updateInfo(newInfo);
                        }
                    }
                }
                break;
#if 0
            case ChanPacket::T_SYNC:
                {
                    unsigned int s = mem.readLong();
                    if ((s-ch->syncPos) != 1)
                    {
                        LOG_CHANNEL("Ch.%d SKIP: %d to %d (%d)",ch->index,ch->syncPos,s,ch->info.numSkips);
                        if (ch->syncPos)
                        {
                            ch->info.numSkips++;
                            if (ch->info.numSkips>50)
                                throw StreamException("Bumped - Too many skips");
                        }
                    }

                    ch->syncPos = s;
                }
                break;
#endif
        }
    }

    return 0;
}

// ------------------------------------------
void RawStream::readHeader(Stream &, Channel *)
{
}

// ------------------------------------------
int RawStream::readPacket(Stream &in, Channel *ch)
{
    readRaw(in, ch);
    return 0;
}

// ------------------------------------------
void RawStream::readEnd(Stream &, Channel *)
{
}

// -----------------------------------
void Channel::getStreamPath(char *str)
{
    char idStr[64];

    getIDStr(idStr);

    sprintf(str, "/stream/%s%s", idStr, info.getTypeExt());
}

// -----------------------------------
bool Channel::writeVariable(Stream &out, const String &var, int index)
{
    char buf[1024];

    buf[0]=0;

    String utf8;

    if (var == "name")
    {
        utf8 = info.name;
        utf8.convertTo(String::T_UNICODESAFE);
        strcpy(buf, utf8.cstr());
    }else if (var == "bitrate")
    {
        sprintf(buf, "%d", info.bitrate);
    }else if (var == "srcrate")
    {
        if (sourceData)
        {
            unsigned int tot = sourceData->getSourceRate();
            sprintf(buf, "%.1f", BYTES_TO_KBPS(tot));
        }else
            strcpy(buf, "0");
    }else if (var == "genre")
    {
        utf8 = info.genre;
        utf8.convertTo(String::T_UNICODESAFE);
        strcpy(buf, utf8.cstr());
    }else if (var == "desc")
    {
        utf8 = info.desc;
        utf8.convertTo(String::T_UNICODESAFE);
        strcpy(buf, utf8.cstr());
    }else if (var == "comment")
    {
        utf8 = info.comment;
        utf8.convertTo(String::T_UNICODESAFE);
        strcpy(buf, utf8.cstr());
    }else if (var == "bcstClap") //JP-MOD
    {
        strcpy(buf, info.ppFlags & ServMgr::bcstClap ? "1":"0");
    }else if (var == "uptime")
    {
        String uptime;
        if (info.lastPlayStart)
            uptime.setFromStopwatch(sys->getTime()-info.lastPlayStart);
        else
            uptime.set("-");
        strcpy(buf, uptime.cstr());
    }
    else if (var == "type")
        sprintf(buf, "%s", info.getTypeStr());
    else if (var == "ext")
        sprintf(buf, "%s", info.getTypeExt());
    else if (var == "proto") {
        switch(info.contentType) {
        case ChanInfo::T_WMA:
        case ChanInfo::T_WMV:
            sprintf(buf, "mms://");
            break;
        default:
            sprintf(buf, "http://");
        }
    }
    else if (var == "localRelays")
        sprintf(buf, "%d", localRelays());
    else if (var == "localListeners")
        sprintf(buf, "%d", localListeners());
    else if (var == "totalRelays")
        sprintf(buf, "%d", totalRelays());
    else if (var == "totalListeners")
        sprintf(buf, "%d", totalListeners());
    else if (var == "totalClaps") //JP-MOD
        sprintf(buf, "%d", totalClaps());
    else if (var == "status")
        sprintf(buf, "%s", getStatusStr());
    else if (var == "keep")
        sprintf(buf, "%s", stayConnected?"Yes":"No");
    else if (var == "id")
        info.id.toStr(buf);
    else if (var.startsWith("track."))
    {
        if (var == "track.title")
            utf8 = info.track.title;
        else if (var == "track.artist")
            utf8 = info.track.artist;
        else if (var == "track.album")
            utf8 = info.track.album;
        else if (var == "track.genre")
            utf8 = info.track.genre;
        else if (var == "track.contactURL")
            utf8 = info.track.contact;

        utf8.convertTo(String::T_UNICODESAFE);
        strcpy(buf,utf8.cstr());

    }else if (var == "contactURL")
        sprintf(buf, "%s", info.url.cstr());
    else if (var == "streamPos")
        sprintf(buf, "%d", streamPos);
    else if (var == "sourceType")
        strcpy(buf, getSrcTypeStr());
    else if (var == "sourceProtocol")
        strcpy(buf, ChanInfo::getProtocolStr(info.srcProtocol));
    else if (var == "sourceURL")
    {
        if (sourceURL.isEmpty())
            sourceHost.host.toStr(buf);
        else
            strcpy(buf, sourceURL.cstr());
    }
    else if (var == "headPos")
        sprintf(buf, "%d", headPack.pos);
    else if (var == "headLen")
        sprintf(buf, "%d", headPack.len);
    else if (var == "numHits")
    {
        ChanHitList *chl = chanMgr->findHitListByID(info.id);
        int numHits = 0;
        if (chl){
//            numHits = chl->numHits();
            ChanHit *hit;
            hit = chl->hit;
            while(hit){
                numHits++;
                hit = hit->next;
            }
        }
        sprintf(buf,"%d",numHits);
    } else if (var == "isBroadcast")
        strcpy(buf, (type == T_BROADCAST) ? "1":"0");
    else
        return false;

    out.writeString(buf);
    return true;
}

// -----------------------------------
// message check
#if 0
                ChanPacket pack;
                MemoryStream mem(pack.data, sizeof(pack.data));
                AtomStream atom(mem);
                atom.writeParent(PCP_BCST, 3);
                    atom.writeChar(PCP_BCST_GROUP, PCP_BCST_GROUP_ALL);
                    atom.writeBytes(PCP_BCST_FROM, servMgr->sessionID.id, 16);
                    atom.writeParent(PCP_MESG, 1);
                        atom.writeString(PCP_MESG_DATA, msg.cstr());

                mem.len = mem.pos;
                mem.rewind();
                pack.len = mem.len;

                GnuID noID;
                noID.clear();

                BroadcastState bcs;
                PCPStream::readAtom(atom, bcs);
                //int cnt = servMgr->broadcastPacketUp(pack, noID, servMgr->sessionID);
                //int cnt = servMgr->broadcastPacketDown(pack, noID, servMgr->sessionID);
                //int cnt = chanMgr->broadcastPacketUp(pack, noID, servMgr->sessionID);
                //LOG_DEBUG("Sent message to %d clients", cnt);
#endif

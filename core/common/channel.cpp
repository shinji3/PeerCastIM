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
#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif

#include "win32/seh.h"

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

// -----------------------------------
void readXMLString(String &str, XML::Node *n, const char *arg)
{
    char *p;
    p = n->findAttr(arg);
    if (p)
    {
        str.set(p,String::T_HTML);
        str.convertTo(String::T_ASCII);
    }
}

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
            case ChanInfo::T_OGG:
            case ChanInfo::T_OGM:
                LOG_CHANNEL("Channel is OGG");
                source = new OGGStream();
                break;
            case ChanInfo::T_FLV:
                LOG_CHANNEL("Channel is FLV");
                source = new FLVStream();
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
const char *ChanInfo::getTypeStr()
{
    if (contentTypeStr.isEmpty()) {
        return getTypeStr(contentType);
    }
    else {
        return contentTypeStr.cstr();
    }
}
// -----------------------------------
const char *ChanInfo::getTypeExt()
{
    if (streamExt.isEmpty()) {
        return getTypeExt(contentType);
    }
    else {
        return streamExt.cstr();
    }
}
// -----------------------------------
const char *ChanInfo::getMIMEType()
{
    if (MIMEType.isEmpty()) {
        return getMIMEType(contentType);
    }
    else {
        return MIMEType.cstr();
    }
}
// -----------------------------------
const char *ChanInfo::getTypeStr(TYPE t)
{
    switch (t)
    {
        case T_RAW:  return "RAW";

        case T_MP3:  return "MP3";
        case T_OGG:  return "OGG";
        case T_OGM:  return "OGM";
        case T_WMA:  return "WMA";

        case T_MOV:  return "MOV";
        case T_MPG:  return "MPG";
        case T_NSV:  return "NSV";
        case T_WMV:  return "WMV";
        case T_FLV:  return "FLV";
        case T_MKV:  return "MKV";
        case T_WEBM: return "WEBM";
        case T_TS:   return "TS";

        case T_PLS:  return "PLS";
        case T_ASX:  return "ASX";

        default: return "UNKNOWN";
    }
}
// -----------------------------------
const char *ChanInfo::getProtocolStr(PROTOCOL t)
{
    switch (t)
    {
        case SP_PEERCAST: return "PEERCAST";
        case SP_HTTP: return "HTTP";
        case SP_FILE: return "FILE";
        case SP_MMS: return "MMS";
        case SP_PCP: return "PCP";
        default: return "UNKNOWN";
    }
}
// -----------------------------------
ChanInfo::PROTOCOL ChanInfo::getProtocolFromStr(const char *str)
{
    if (stricmp(str,"PEERCAST")==0)
        return SP_PEERCAST;
    else if (stricmp(str,"HTTP")==0)
        return SP_HTTP;
    else if (stricmp(str,"FILE")==0)
        return SP_FILE;
    else if (stricmp(str,"MMS")==0)
        return SP_MMS;
    else if (stricmp(str,"PCP")==0)
        return SP_PCP;
    else 
        return SP_UNKNOWN;
}

// -----------------------------------
const char *ChanInfo::getTypeExt(TYPE t)
{
    switch(t)
    {
        case ChanInfo::T_OGM:
        case ChanInfo::T_OGG:
            return ".ogg";
        case ChanInfo::T_MP3:
            return ".mp3";
        case ChanInfo::T_MOV:
            return ".mov";
        case ChanInfo::T_NSV:
            return ".nsv";
        case ChanInfo::T_WMV:
            return ".wmv";
        case ChanInfo::T_WMA:
            return ".wma";
        case ChanInfo::T_FLV:
            return ".flv";
        case ChanInfo::T_MKV:
            return ".mkv";
        case ChanInfo::T_WEBM:
            return ".webm";
        case ChanInfo::T_TS:
            return ".ts";
        default:
            return "";
    }
}
// -----------------------------------
const char *ChanInfo::getMIMEType(TYPE t)
{
    switch(t)
    {
        case ChanInfo::T_OGG:
            return MIME_XOGG;
        case ChanInfo::T_OGM:
            return MIME_XOGG;
        case ChanInfo::T_MP3:
            return MIME_MP3;
        case ChanInfo::T_MOV:
            return MIME_MOV;
        case ChanInfo::T_MPG:
            return MIME_MPG;
        case ChanInfo::T_NSV:
            return MIME_NSV;
        case ChanInfo::T_ASX:
            return MIME_ASX;
        case ChanInfo::T_WMA:
            return MIME_WMA;
        case ChanInfo::T_WMV:
            return MIME_WMV;
        case ChanInfo::T_FLV:
            return MIME_FLV;
        case ChanInfo::T_MKV:
            return MIME_MKV;
        case ChanInfo::T_WEBM:
            return MIME_WEBM;
        case ChanInfo::T_TS:
            return MIME_TS;
        default:
            return "application/octet-stream";
    }
}
// -----------------------------------
ChanInfo::TYPE ChanInfo::getTypeFromStr(const char *str)
{
    if (stricmp(str,"MP3")==0)
        return T_MP3;
    else if (stricmp(str,"OGG")==0)
        return T_OGG;
    else if (stricmp(str,"OGM")==0)
        return T_OGM;
    else if (stricmp(str,"RAW")==0)
        return T_RAW;
    else if (stricmp(str,"NSV")==0)
        return T_NSV;
    else if (stricmp(str,"WMA")==0)
        return T_WMA;
    else if (stricmp(str,"WMV")==0)
        return T_WMV;
    else if (stricmp(str,"FLV")==0)
        return T_FLV;
    else if (stricmp(str,"MKV")==0)
        return T_MKV;
    else if (stricmp(str,"WEBM")==0)
        return T_WEBM;
    else if (stricmp(str,"TS")==0)
        return T_TS;
    else if (stricmp(str,"PLS")==0)
        return T_PLS;
    else if (stricmp(str,"M3U")==0)
        return T_PLS;
    else if (stricmp(str,"ASX")==0)
        return T_ASX;
    else 
        return T_UNKNOWN;
}
// -----------------------------------
bool    ChanInfo::matchNameID(ChanInfo &inf)
{
    if (inf.id.isSet())
        if (id.isSame(inf.id))
            return true;

    if (!inf.name.isEmpty())
        if (name.contains(inf.name))
            return true;

    return false;
}
// -----------------------------------
bool    ChanInfo::match(ChanInfo &inf)
{
    bool matchAny=true;

    if (inf.status != S_UNKNOWN)
    {
        if (status != inf.status)
            return false;
    }

    if (inf.bitrate != 0)
    {
        if (bitrate == inf.bitrate)
            return true;
        matchAny = false;
    }

    if (inf.id.isSet())
    {
        if (id.isSame(inf.id))
            return true;
        matchAny = false;
    }

    if (inf.contentType != T_UNKNOWN)
    {
        if (contentType == inf.contentType)
            return true;
        matchAny = false;
    }

    if (!inf.name.isEmpty())
    {
        if (name.contains(inf.name))
            return true;
        matchAny = false;
    }

    if (!inf.genre.isEmpty())
    {
        if (genre.contains(inf.genre))
            return true;
        matchAny = false;
    }

    return matchAny;
}
// -----------------------------------
bool TrackInfo::update(const TrackInfo &inf)
{
    bool changed = false;

    if (!contact.isSame(inf.contact))
    {
        contact = inf.contact;
        changed = true;
    }

    if (!title.isSame(inf.title))
    {
        title = inf.title;
        changed = true;
    }

    if (!artist.isSame(inf.artist))
    {
        artist = inf.artist;
        changed = true;
    }

    if (!album.isSame(inf.album))
    {
        album = inf.album;
        changed = true;
    }

    if (!genre.isSame(inf.genre))
    {
        genre = inf.genre;
        changed = true;
    }


    return changed;
}


// -----------------------------------
bool ChanInfo::update(const ChanInfo &info)
{
    bool changed = false;



    // check valid id
    if (!info.id.isSet())
        return false;

    // only update from chaninfo that has full name etc..
    if (info.name.isEmpty())
        return false;

    // check valid broadcaster key
    if (bcID.isSet())
    {
        if (!bcID.isSame(info.bcID))
        {
            LOG_ERROR("ChanInfo BC key not valid");
            return false;
        }
    }else
    {
        bcID = info.bcID;
    }



    if (bitrate != info.bitrate)
    {
        bitrate = info.bitrate;
        changed = true;
    }

    if (contentType != info.contentType)
    {
        contentType = info.contentType;
        changed = true;
    }

    if (!contentTypeStr.isSame(info.contentTypeStr))
    {
        contentTypeStr = info.contentTypeStr;
        changed = true;
    }

    if (!MIMEType.isSame(info.MIMEType))
    {
        MIMEType = info.MIMEType;
        changed = true;
    }

    if (!streamExt.isSame(info.streamExt))
    {
        streamExt = info.streamExt;
        changed = true;
    }

    if(ppFlags != info.ppFlags) //JP-MOD
    {
        ppFlags = info.ppFlags;
        changed = true;
    }

    if (!desc.isSame(info.desc)) //JP-EX
    {
        desc = info.desc;
        changed = true;
    }

    if (!name.isSame(info.name))
    {
        name = info.name;
        changed = true;
    }

    if (!comment.isSame(info.comment))
    {
        comment = info.comment;
        changed = true;
    }

    if (!genre.isSame(info.genre))
    {
        genre = info.genre;
        changed = true;
    }
    
    if (!url.isSame(info.url))
    {
        url = info.url;
        changed = true;
    }

    if (track.update(info.track))
        changed = true;


    return changed;
}
// -----------------------------------
void ChanInfo::initNameID(const char *n)
{
    init();
    id.fromStr(n);
    if (!id.isSet())
        name.set(n);
}

// -----------------------------------
void ChanInfo::init()
{
    status = S_UNKNOWN;
    name.clear();
    bitrate = 0;
    contentType = T_UNKNOWN;
    contentTypeStr.clear();
    MIMEType.clear();
    streamExt.clear();
    srcProtocol = SP_UNKNOWN;
    id.clear();
    url.clear();
    genre.clear();
    comment.clear();
    track.clear();
    lastPlayStart = 0;
    lastPlayEnd = 0;
    numSkips = 0;
    bcID.clear();
    createdTime = 0;
    ppFlags = 0; //JP-MOD
}
// -----------------------------------
void ChanInfo::readTrackXML(XML::Node *n)
{
    track.clear();
    readXMLString(track.title,n,"title");
    readXMLString(track.contact,n,"contact");
    readXMLString(track.artist,n,"artist");
    readXMLString(track.album,n,"album");
    readXMLString(track.genre,n,"genre");
}
// -----------------------------------
unsigned int ChanInfo::getUptime()
{
    // calculate uptime and cap if requested by settings.
    unsigned int upt;
    upt = lastPlayStart?(sys->getTime()-lastPlayStart):0;
    if (chanMgr->maxUptime)
        if (upt > chanMgr->maxUptime)
            upt = chanMgr->maxUptime;
    return upt;
}
// -----------------------------------
unsigned int ChanInfo::getAge()
{
    return sys->getTime()-createdTime;
}

// ------------------------------------------
void ChanInfo::readTrackAtoms(AtomStream &atom,int numc)
{
    for(int i=0; i<numc; i++)
    {
        int c,d;
        ID4 id = atom.read(c,d);
        if (id == PCP_CHAN_TRACK_TITLE)
        {
            atom.readString(track.title.data,sizeof(track.title.data),d);
        }else if (id == PCP_CHAN_TRACK_CREATOR)
        {
            atom.readString(track.artist.data,sizeof(track.artist.data),d);
        }else if (id == PCP_CHAN_TRACK_URL)
        {
            atom.readString(track.contact.data,sizeof(track.contact.data),d);
        }else if (id == PCP_CHAN_TRACK_ALBUM)
        {
            atom.readString(track.album.data,sizeof(track.album.data),d);
        }else
            atom.skip(c,d);
    }
}
// ------------------------------------------
void ChanInfo::readInfoAtoms(AtomStream &atom,int numc)
{
    for(int i=0; i<numc; i++)
    {
        int c,d;
        ID4 id = atom.read(c,d);
        if (id == PCP_CHAN_INFO_NAME)
        {
            atom.readString(name.data,sizeof(name.data),d);
        }else if (id == PCP_CHAN_INFO_BITRATE)
        {
            bitrate = atom.readInt();
        }else if (id == PCP_CHAN_INFO_GENRE)
        {
            atom.readString(genre.data,sizeof(genre.data),d);
        }else if (id == PCP_CHAN_INFO_URL)
        {
            atom.readString(url.data,sizeof(url.data),d);
        }else if (id == PCP_CHAN_INFO_DESC)
        {
            atom.readString(desc.data,sizeof(desc.data),d);
        }else if (id == PCP_CHAN_INFO_COMMENT)
        {
            atom.readString(comment.data,sizeof(comment.data),d);
        }else if (id == PCP_CHAN_INFO_TYPE)
        {
            char type[16];
            atom.readString(type,sizeof(type),d);
            contentType = ChanInfo::getTypeFromStr(type);
            contentTypeStr = type;
        }else if (id == PCP_CHAN_INFO_STREAMTYPE)
        {
            atom.readString(MIMEType.data,sizeof(MIMEType.data),d);
        }else if (id == PCP_CHAN_INFO_STREAMEXT)
        {
            atom.readString(streamExt.data,sizeof(streamExt.data),d);
        }else if (id == PCP_CHAN_INFO_PPFLAGS) //JP-MOD
        {
            ppFlags = (unsigned int)atom.readInt();
        }else
            atom.skip(c,d);
    }    
}

// -----------------------------------
void ChanInfo::writeInfoAtoms(AtomStream &atom)
{
    int natoms = 7;

    natoms += !MIMEType.isEmpty();
    natoms += !streamExt.isEmpty();
    natoms += ppFlags ? 1 : 0; //JP-MOD

    atom.writeParent(PCP_CHAN_INFO, natoms);
        atom.writeString(PCP_CHAN_INFO_NAME, name.cstr());
        atom.writeInt(PCP_CHAN_INFO_BITRATE, bitrate);
        atom.writeString(PCP_CHAN_INFO_GENRE, genre.cstr());
        atom.writeString(PCP_CHAN_INFO_URL, url.cstr());
        atom.writeString(PCP_CHAN_INFO_DESC, desc.cstr());
        atom.writeString(PCP_CHAN_INFO_COMMENT, comment.cstr());
        atom.writeString(PCP_CHAN_INFO_TYPE, getTypeStr());
        if (!MIMEType.isEmpty())
            atom.writeString(PCP_CHAN_INFO_STREAMTYPE, MIMEType.cstr());
        if (!streamExt.isEmpty())
            atom.writeString(PCP_CHAN_INFO_STREAMEXT, streamExt.cstr());
        if (ppFlags)
            atom.writeInt(PCP_CHAN_INFO_PPFLAGS, ppFlags); //JP-MOD
}
// -----------------------------------
void ChanInfo::writeTrackAtoms(AtomStream &atom)
{
    atom.writeParent(PCP_CHAN_TRACK,4);
        atom.writeString(PCP_CHAN_TRACK_TITLE,track.title.cstr());
        atom.writeString(PCP_CHAN_TRACK_CREATOR,track.artist.cstr());
        atom.writeString(PCP_CHAN_TRACK_URL,track.contact.cstr());
        atom.writeString(PCP_CHAN_TRACK_ALBUM,track.album.cstr());
}


// -----------------------------------
XML::Node *ChanInfo::createChannelXML()
{
    char idStr[64];

    String nameUNI = name;
    nameUNI.convertTo(String::T_UNICODESAFE);

    String urlUNI = url;
    urlUNI.convertTo(String::T_UNICODESAFE);

    String genreUNI = genre;
    genreUNI.convertTo(String::T_UNICODESAFE);

    String descUNI = desc;
    descUNI.convertTo(String::T_UNICODESAFE);

    String commentUNI;
    commentUNI = comment;
    commentUNI.convertTo(String::T_UNICODESAFE);


    id.toStr(idStr);


    return new XML::Node("channel name=\"%s\" id=\"%s\" bitrate=\"%d\" type=\"%s\" genre=\"%s\" desc=\"%s\" url=\"%s\" uptime=\"%d\" comment=\"%s\" skips=\"%d\" age=\"%d\" bcflags=\"%d\"",
        nameUNI.cstr(),
        idStr,
        bitrate,
        getTypeStr(),
        genreUNI.cstr(),
        descUNI.cstr(),
        urlUNI.cstr(),
        getUptime(),
        commentUNI.cstr(),
        numSkips,
        getAge(),
        bcID.getFlags()
        );    
}

// -----------------------------------
XML::Node *ChanInfo::createQueryXML()
{
    char buf[512];
    char idStr[64];


    String nameHTML = name;
    nameHTML.convertTo(String::T_HTML);
    String genreHTML = genre;
    genreHTML.convertTo(String::T_HTML);

    buf[0]=0;
    if (!nameHTML.isEmpty())
    {
        strcat(buf," name=\"");
        strcat(buf,nameHTML.cstr());
        strcat(buf,"\"");
    }

    if (!genreHTML.isEmpty())
    {
        strcat(buf," genre=\"");
        strcat(buf,genreHTML.cstr());
        strcat(buf,"\"");
    }

    if (id.isSet())
    {
        id.toStr(idStr);
        strcat(buf," id=\"");
        strcat(buf,idStr);
        strcat(buf,"\"");
    }
        

    return new XML::Node("channel %s",buf);
}

// -----------------------------------
XML::Node *ChanInfo::createRelayChannelXML()
{
    char idStr[64];

    id.toStr(idStr);


    return new XML::Node("channel id=\"%s\" uptime=\"%d\" skips=\"%d\" age=\"%d\"",
        idStr,
        getUptime(),
        numSkips,
        getAge()
        );    
}// -----------------------------------
XML::Node *ChanInfo::createTrackXML()
{
    String titleUNI = track.title;
    titleUNI.convertTo(String::T_UNICODESAFE);

    String artistUNI = track.artist;
    artistUNI.convertTo(String::T_UNICODESAFE);

    String albumUNI = track.album;
    albumUNI.convertTo(String::T_UNICODESAFE);

    String genreUNI = track.genre;
    genreUNI.convertTo(String::T_UNICODESAFE);

    String contactUNI = track.contact;
    contactUNI.convertTo(String::T_UNICODESAFE);
    


    return new XML::Node("track title=\"%s\" artist=\"%s\" album=\"%s\" genre=\"%s\" contact=\"%s\"",
        titleUNI.cstr(),
        artistUNI.cstr(),
        albumUNI.cstr(),
        genreUNI.cstr(),
        contactUNI.cstr()
        );
}

// -----------------------------------
void ChanInfo::init(XML::Node *n)
{
    init();

    updateFromXML(n);
}
// -----------------------------------
void ChanInfo::updateFromXML(XML::Node *n)
{
    String typeStr,idStr;

    readXMLString(name,n,"name");
    readXMLString(genre,n,"genre");
    readXMLString(url,n,"url");
    readXMLString(desc,n,"desc");


    int br = n->findAttrInt("bitrate");
    if (br)
        bitrate = br;

    { //JP-MOD
        ppFlags = ServMgr::bcstNone;

        if (n->findAttrInt("bcstClap"))
            ppFlags |= ServMgr::bcstClap;
    }

    readXMLString(typeStr,n,"type");
    if (!typeStr.isEmpty()) {
         contentType = getTypeFromStr(typeStr.cstr());
        contentTypeStr = typeStr;
    }

    readXMLString(idStr,n,"id");
    if (!idStr.isEmpty())
        id.fromStr(idStr.cstr());

    readXMLString(comment,n,"comment");

    XML::Node *tn = n->findNode("track");
    if (tn)
        readTrackXML(tn);

}

// -----------------------------------
void ChanInfo::init(const char *n, GnuID &cid, TYPE tp, int br)
{
    init();

    name.set(n);
    bitrate = br;
    contentType = tp;
    id = cid;
}

// -----------------------------------
void ChanInfo::init(const char *fn)
{
    init();

    if (fn)
        name.set(fn);
}
// -----------------------------------
void PlayList::readASX(Stream &in)
{
    LOG_DEBUG("Reading ASX");
    XML xml;

    try
    {
        xml.read(in);
    }catch(StreamException &) {} // TODO: eof is NOT handled properly in sockets - always get error at end

    if (xml.root)
    {
        XML::Node *n = xml.root->child;
        while (n)
        {
            if (stricmp("entry",n->getName())==0)
            {
                XML::Node *rf = n->findNode("ref");
                if (rf)
                {
                    char *hr = rf->findAttr("href");
                    if (hr)
                    {
                        addURL(hr,"","");
                        //LOG("asx url %s",hr);
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
    while (in.readLine(tmp,sizeof(tmp)))
    {
        if (strnicmp(tmp,"file",4)==0)
        {
            char *p = strstr(tmp,"=");
            if (p)
                addURL(p+1,"","");
        }
    }
}
// -----------------------------------
void PlayList::readPLS(Stream &in)
{
    char tmp[256];
    while (in.readLine(tmp,sizeof(tmp)))
    {
        if (tmp[0] != '#')
            addURL(tmp,"","");
    }
}
// -----------------------------------
void PlayList::writeSCPLS(Stream &out)
{
    out.writeLine("[playlist]");
    out.writeLine("");
    out.writeLineF("NumberOfEntries=%d",numURLs);

    for(int i=0; i<numURLs; i++)
    {
        out.writeLineF("File%d=%s",i+1,urls[i].cstr());
        out.writeLineF("Title%d=%s",i+1,titles[i].cstr());
        out.writeLineF("Length%d=-1",i+1);
    }
    out.writeLine("Version=2");
}
// -----------------------------------
void PlayList::writePLS(Stream &out)
{
    for(int i=0; i<numURLs; i++)
        out.writeLineF("%s",urls[i].cstr());
}
// -----------------------------------
void PlayList::writeRAM(Stream &out)
{
    for(int i=0; i<numURLs; i++)
        out.writeLineF("%s",urls[i].cstr());
}

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
void PlayList::writeASX(Stream &out)
{
    out.writeLine("<ASX Version=\"3.0\">");

    String::TYPE tEncoding = String::T_SJIS;
    if(servMgr->asxDetailedMode == 2)
    {
        out.writeLine("<PARAM NAME = \"Encoding\" VALUE = \"utf-8\" />"); //JP-MOD Memo: UTF-8 cannot be used in some recording software.
        tEncoding = String::T_UNICODESAFE;
    }

    if(servMgr->asxDetailedMode)
        WriteASXInfo(out, titles[0], contacturls[0], tEncoding); //JP-MOD

    for(int i=0; i<numURLs; i++)
    {
        out.writeLine("<ENTRY>");
        if(servMgr->asxDetailedMode)
            WriteASXInfo(out, titles[i], contacturls[i], tEncoding); //JP-MOD
        out.writeLineF("<REF href = \"%s\" />",urls[i].cstr());
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

    sprintf(url.cstr(),"%s/stream/%s%s",path,nid,info.getTypeExt());
    addURL(url.cstr(),info.name,info.url);
}

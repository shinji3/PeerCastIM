#include "chanmgr.h"

#include "playlist.h"
#include "peercast.h"
#include "version2.h" // PCP_BROADCAST_FLAGS
#include "win32/seh.h"

// -----------------------------------
void ChanMgr::startSearch(ChanInfo &info)
{
    searchInfo = info;
    clearHitLists();
    numFinds = 0;
    lastHit = 0;
//  lastSearch = 0;
    searchActive = true;
}

// -----------------------------------
void ChanMgr::quit()
{
    LOG_DEBUG("ChanMgr is quitting..");
    closeAll();
}

// -----------------------------------
int ChanMgr::numIdleChannels()
{
    int cnt = 0;
    Channel *ch = channel;
    while (ch)
    {
        if (ch->isActive())
            if (ch->thread.active)
                if (ch->status == Channel::S_IDLE)
                    cnt++;
        ch = ch->next;
    }
    return cnt;
}

// -----------------------------------
void ChanMgr::closeOldestIdle()
{
    unsigned int idleTime = (unsigned int)-1;
    Channel *ch = channel, *oldest = NULL;
    while (ch)
    {
        if (ch->isActive())
            if (ch->thread.active)
                if (ch->status == Channel::S_IDLE)
                    if (ch->lastIdleTime < idleTime)
                    {
                        oldest = ch;
                        idleTime = ch->lastIdleTime;
                    }
        ch = ch->next;
    }

    if (oldest)
        oldest->thread.active = false;
}

// -----------------------------------
void ChanMgr::closeAll()
{
    Channel *ch = channel;
    while (ch)
    {
        if (ch->thread.active)
            ch->thread.shutdown();
        ch = ch->next;
    }
}

// -----------------------------------
Channel *ChanMgr::findChannelByNameID(ChanInfo &info)
{
    Channel *ch = channel;
    while (ch)
    {
        if (ch->isActive())
            if (ch->info.matchNameID(info))
                return ch;
        ch = ch->next;
    }
    return NULL;
}

// -----------------------------------
Channel *ChanMgr::findChannelByName(const char *n)
{
    Channel *ch = channel;
    while (ch)
    {
        if (ch->isActive())
            if (stricmp(ch->info.name, n) == 0)
                return ch;
        ch = ch->next;
    }

    return NULL;
}

// -----------------------------------
Channel *ChanMgr::findChannelByIndex(int index)
{
    int cnt = 0;
    Channel *ch = channel;
    while (ch)
    {
        if (ch->isActive())
        {
            if (cnt == index)
                return ch;
            cnt++;
        }
        ch = ch->next;
    }
    return NULL;
}

// -----------------------------------
Channel *ChanMgr::findChannelByMount(const char *str)
{
    Channel *ch = channel;
    while (ch)
    {
        if (ch->isActive())
            if (strcmp(ch->mount, str) == 0)
                return ch;
        ch = ch->next;
    }

    return NULL;
}

// -----------------------------------
Channel *ChanMgr::findChannelByID(const GnuID &id)
{
    Channel *ch = channel;
    while (ch)
    {
        if (ch->isActive())
            if (ch->info.id.isSame(id))
                return ch;
        ch = ch->next;
    }
    return NULL;
}

// -----------------------------------
Channel *ChanMgr::findChannelByChannelID(int id)
{
    int cnt=0;
    Channel *ch = channel;
    while (ch)
    {
        if (ch->isActive()){
            if (ch->channel_id == id){
                return ch;
            }
        }
        ch = ch->next;
    }
    return NULL;
}
// -----------------------------------
int ChanMgr::findChannels(ChanInfo &info, Channel **chlist, int max)
{
    int cnt = 0;
    Channel *ch = channel;
    while (ch)
    {
        if (ch->isActive())
            if (ch->info.match(info))
            {
                chlist[cnt++] = ch;
                if (cnt >= max)
                    break;
            }
        ch = ch->next;
    }
    return cnt;
}

// -----------------------------------
int ChanMgr::findChannelsByStatus(Channel **chlist, int max, Channel::STATUS status)
{
    int cnt = 0;
    Channel *ch = channel;
    while (ch)
    {
        if (ch->isActive())
            if (ch->status == status)
            {
                chlist[cnt++] = ch;
                if (cnt >= max)
                    break;
            }
        ch = ch->next;
    }
    return cnt;
}

// -----------------------------------
Channel *ChanMgr::createRelay(ChanInfo &info, bool stayConnected)
{
    Channel *c = chanMgr->createChannel(info, NULL);
    if (c)
    {
        c->stayConnected = stayConnected;
        c->startGet();
        return c;
    }
    return NULL;
}

// -----------------------------------
Channel *ChanMgr::findAndRelay(ChanInfo &info)
{
    char idStr[64];
    info.id.toStr(idStr);
    LOG_CHANNEL("Searching for: %s (%s)", idStr, info.name.cstr());

    if(!isIndexTxt(&info))    // for PCRaw (popup)
        peercastApp->notifyMessage(ServMgr::NT_PEERCAST,"Finding channel...");

    Channel *c = NULL;

    WLockBlock wb(&(chanMgr->channellock));

    wb.on();

    c = findChannelByNameID(info);

    if (!c)
    {
        c = chanMgr->createChannel(info, NULL);
        if (c)
        {
            c->setStatus(Channel::S_SEARCHING);
            c->startGet();
        }
    } else if (!(c->thread.active)){
        c->thread.active = true;
        c->thread.finish = false;
        c->info.lastPlayStart = 0; // reconnect
        c->info.lastPlayEnd = 0;
        if (c->finthread){
            c->finthread->finish = true;
            c->finthread = NULL;
        }
        if (c->status != Channel::S_CONNECTING && c->status != Channel::S_SEARCHING){
            c->setStatus(Channel::S_SEARCHING);
            c->startGet();
        }
    }

    wb.off();

    for (int i = 0; i < 600; i++)    // search for 1 minute.
    {
        wb.on();
        c = findChannelByNameID(info);

        if (!c)
        {
//          peercastApp->notifyMessage(ServMgr::NT_PEERCAST,"Channel not found");
            return NULL;
        }

        if (c->isPlaying() && (c->info.contentType != ChanInfo::T_UNKNOWN))
            break;

        wb.off();

        sys->sleep(100);
    }

    return c;
}

// -----------------------------------
ChanMgr::ChanMgr()
{
    channel = NULL;

    hitlist = NULL;

    currFindAndPlayChannel.clear();

    broadcastMsg.clear();
    broadcastMsgInterval = 10;

    broadcastID.generate(PCP_BROADCAST_FLAGS);

    deadHitAge = 600;

    icyIndex = 0;
    icyMetaInterval = 8192;
    maxRelaysPerChannel = 1;

    searchInfo.init();

    minBroadcastTTL = 1;
    maxBroadcastTTL = 7;

    pushTimeout = 60;   // 1 minute
    pushTries = 5;      // 5 times
    maxPushHops = 8;    // max 8 hops away
    maxUptime = 0;      // 0 = none

    prefetchTime = 10;  // n seconds

    hostUpdateInterval = 120; // 2 minutes

    bufferTime = 5;

    autoQuery = 0;
    lastQuery = 0;

    lastYPConnect = 0;
    lastYPConnect2 = 0;
}

// -----------------------------------
bool ChanMgr::writeVariable(Stream &out, const String &var, int index)
{
    char buf[1024];
    if (var == "numHitLists")
        sprintf(buf, "%d", numHitLists());

    else if (var == "numChannels")
        sprintf(buf, "%d", numChannels());
    else if (var == "djMessage")
    {
        String utf8 = broadcastMsg;
        utf8.convertTo(String::T_UNICODESAFE);
        strcpy(buf, utf8.cstr());
    }
    else if (var == "icyMetaInterval")
        sprintf(buf, "%d", icyMetaInterval);
    else if (var == "maxRelaysPerChannel")
        sprintf(buf, "%d", maxRelaysPerChannel);
    else if (var == "hostUpdateInterval")
        sprintf(buf, "%d", hostUpdateInterval);
    else if (var == "broadcastID")
        broadcastID.toStr(buf);
    else
        return false;

    out.writeString(buf);
    return true;
}

// -----------------------------------
void ChanMgr::broadcastTrackerUpdate(GnuID &svID, bool force)
{
    Channel *c = channel;
    while (c)
    {
        if ( c->isActive() && c->isBroadcasting() )
            c->broadcastTrackerUpdate(svID, force);
        c = c->next;
    }
}

// -----------------------------------
int ChanMgr::broadcastPacketUp(ChanPacket &pack, GnuID &chanID, GnuID &srcID, GnuID &destID)
{
    int cnt = 0;

    Channel *c = channel;
    while (c)
    {
        if (c->sendPacketUp(pack, chanID, srcID, destID))
            cnt++;
        c = c->next;
    }

    return cnt;
}

// -----------------------------------
void ChanMgr::broadcastRelays(Servent *serv, int minTTL, int maxTTL)
{
    //if ((servMgr->getFirewall() == ServMgr::FW_OFF) || servMgr->serverHost.localIP())
    {
        Host sh = servMgr->serverHost;
        bool push = (servMgr->getFirewall() != ServMgr::FW_OFF);
        bool busy = (servMgr->pubInFull() && servMgr->outFull()) || servMgr->relaysFull();
        bool stable = servMgr->totalStreams>0;

        GnuPacket hit;

        int numChans = 0;

        Channel *c = channel;
        while (c)
        {
            if (c->isPlaying())
            {
                bool tracker = c->isBroadcasting();

                int ttl = (c->info.getUptime() / servMgr->relayBroadcast);  // 1 hop per N seconds

                if (ttl < minTTL)
                    ttl = minTTL;

                if (ttl > maxTTL)
                    ttl = maxTTL;

                if (hit.initHit(sh, c, NULL, push, busy, stable, tracker, ttl))
                {
                    int numOut = 0;
                    numChans++;
                    if (serv)
                    {
                        serv->outputPacket(hit, false);
                        numOut++;
                    }

                    LOG_NETWORK("Sent channel to %d servents, TTL %d", numOut, ttl);
                }
            }
            c = c->next;
        }
        //if (numChans)
        //  LOG_NETWORK("Sent %d channels to %d servents", numChans, numOut);
    }
}

// -----------------------------------
void ChanMgr::setUpdateInterval(unsigned int v)
{
    hostUpdateInterval = v;
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
// -----------------------------------
void ChanMgr::setBroadcastMsg(String &msg)
{
    if (!msg.isSame(broadcastMsg))
    {
        broadcastMsg = msg;

        Channel *c = channel;
        while (c)
        {
            if (c->isActive() && c->isBroadcasting())
            {
                ChanInfo newInfo = c->info;
                newInfo.comment = broadcastMsg;
                c->updateInfo(newInfo);
            }
            c = c->next;
        }
    }
}

// -----------------------------------
void ChanMgr::clearHitLists()
{
//  LOG_DEBUG("clearHitLists HITLISTLOCK ON-------------");
    chanMgr->hitlistlock.on();
    while (hitlist)
    {
        peercastApp->delChannel(&hitlist->info);

        ChanHitList *next = hitlist->next;

        delete hitlist;

        hitlist = next;
    }
//  LOG_DEBUG("clearHitLists HITLISTLOCK OFF-------------");
    chanMgr->hitlistlock.off();
}

// -----------------------------------
Channel *ChanMgr::deleteChannel(Channel *delchan)
{
    lock.on();

    Channel *ch = channel, *prev = NULL, *next = NULL;

    while (ch)
    {
        if (ch == delchan)
        {
            Channel *next = ch->next;
            if (prev)
                prev->next = next;
            else
                channel = next;

            if (delchan->sourceStream){
                delchan->sourceStream->parent = NULL;
            }

            delete delchan;

            break;
        }
        prev = ch;
        ch = ch->next;
    }

    lock.off();
    return next;
}

// -----------------------------------
Channel *ChanMgr::createChannel(ChanInfo &info, const char *mount)
{
    lock.on();

    Channel *nc = NULL;

    nc = new Channel();

    nc->next = channel;
    channel = nc;

    nc->info = info;
    nc->info.lastPlayStart = 0;
    nc->info.lastPlayEnd = 0;
    nc->info.status = ChanInfo::S_UNKNOWN;
    if (mount)
        nc->mount.set(mount);
    nc->setStatus(Channel::S_WAIT);
    nc->type = Channel::T_ALLOCATED;
    nc->info.createdTime = sys->getTime();

    LOG_CHANNEL("New channel created");

    lock.off();
    return nc;
}

// -----------------------------------
int ChanMgr::pickHits(ChanHitSearch &chs)
{
    ChanHitList *chl = hitlist;
    while (chl)
    {
        if (chl->isUsed())
            if (chl->pickHits(chs))
            {
                chl->info.id;
                return 1;
            }
        chl = chl->next;
    }
    return 0;
}

// -----------------------------------
ChanHitList *ChanMgr::findHitList(ChanInfo &info)
{
    ChanHitList *chl = hitlist;
    while (chl)
    {
        if (chl->isUsed())
            if (chl->info.matchNameID(info))
                return chl;

        chl = chl->next;
    }
    return NULL;
}

// -----------------------------------
ChanHitList *ChanMgr::findHitListByID(GnuID &id)
{
    ChanHitList *chl = hitlist;
    while (chl)
    {
        if (chl->isUsed())
            if (chl->info.id.isSame(id))
                return chl;
        chl = chl->next;
    }
    return NULL;
}

// -----------------------------------
int ChanMgr::numHitLists()
{
    int num = 0;
    ChanHitList *chl = hitlist;
    while (chl)
    {
        if (chl->isUsed())
            num++;
        chl = chl->next;
    }
    return num;
}

// -----------------------------------
ChanHitList *ChanMgr::addHitList(ChanInfo &info)
{
    ChanHitList *chl = new ChanHitList();

    chl->next = hitlist;
    hitlist = chl;

    chl->used = true;
    chl->info = info;
    chl->info.createdTime = sys->getTime();
    peercastApp->addChannel(&chl->info);

    return chl;
}

// -----------------------------------
void ChanMgr::clearDeadHits(bool clearTrackers)
{
    unsigned int interval;

    if (servMgr->isRoot)
        interval = 1200;        // mainly for old 0.119 clients
    else
        interval = hostUpdateInterval+120;

    chanMgr->hitlistlock.on();
    ChanHitList *chl = hitlist, *prev = NULL;
    while (chl)
    {
        if (chl->isUsed())
        {
            if (chl->clearDeadHits(interval, clearTrackers) == 0)
            {
                if (!isBroadcasting(chl->info.id))
                {
                    if (!chanMgr->findChannelByID(chl->info.id))
                    {
                        peercastApp->delChannel(&chl->info);

                        ChanHitList *next = chl->next;
                        if (prev)
                            prev->next = next;
                        else
                            hitlist = next;

                        delete chl;
                        chl = next;
                        continue;
                    }
                }
            }
        }
        prev = chl;
        chl = chl->next;
    }
    chanMgr->hitlistlock.off();
}

// -----------------------------------
bool    ChanMgr::isBroadcasting(GnuID &id)
{
    Channel *ch = findChannelByID(id);
    if (ch)
        return ch->isBroadcasting();

    return false;
}

// -----------------------------------
bool    ChanMgr::isBroadcasting()
{
    Channel *ch = channel;
    while (ch)
    {
        if (ch->isActive())
            if (ch->isBroadcasting())
                return true;

        ch = ch->next;
    }
    return false;
}

// -----------------------------------
int ChanMgr::numChannels()
{
    int tot = 0;
    Channel *ch = channel;
    while (ch)
    {
        if (ch->isActive())
            tot++;
        ch = ch->next;
    }
    return tot;
}

// -----------------------------------
void ChanMgr::deadHit(ChanHit &hit)
{
    ChanHitList *chl = findHitListByID(hit.chanID);
    if (chl)
        chl->deadHit(hit);
}

// -----------------------------------
void ChanMgr::delHit(ChanHit &hit)
{
    ChanHitList *chl = findHitListByID(hit.chanID);
    if (chl)
        chl->delHit(hit);
}

// -----------------------------------
void ChanMgr::addHit(Host &h, GnuID &id, bool tracker)
{
    ChanHit hit;
    hit.init();
    hit.host = h;
    hit.rhost[0] = h;
    hit.rhost[1].init();
    hit.tracker = tracker;
    hit.recv = true;
    hit.chanID = id;
    addHit(hit);
}

// -----------------------------------
ChanHit *ChanMgr::addHit(ChanHit &h)
{
    if (searchActive)
        lastHit = sys->getTime();

    ChanHitList *hl = NULL;

    hl = findHitListByID(h.chanID);

    if (!hl)
    {
        ChanInfo info;
        info.id = h.chanID;
        hl = addHitList(info);
    }

    if (hl)
    {
        return hl->addHit(h);
    }else
        return NULL;
}

// -----------------------------------
bool ChanMgr::findParentHit(ChanHit &p)
{
    ChanHitList *hl=NULL;

    chanMgr->hitlistlock.on();

    hl = findHitListByID(p.chanID);

    if (hl)
    {
        ChanHit *ch = hl->hit;
        while (ch)
        {
            if (!ch->dead && (ch->rhost[0].ip == p.uphost.ip)
                && (ch->rhost[0].port == p.uphost.port))
            {
                chanMgr->hitlistlock.off();
                return 1;
            }
            ch = ch->next;
        }
    }

    chanMgr->hitlistlock.off();

    return 0;
}

// -----------------------------------
class ChanFindInfo : public ThreadInfo
{
public:
    ChanInfo    info;
    bool        keep;
};

// -----------------------------------
THREAD_PROC findAndPlayChannelProcMain(ThreadInfo *th)
{
    ChanFindInfo *cfi = (ChanFindInfo *)th;

    ChanInfo info;
    info = cfi->info;

    Channel *ch = chanMgr->findChannelByNameID(info);

    chanMgr->currFindAndPlayChannel = info.id;

    if (!ch)
        ch = chanMgr->findAndRelay(info);

    if (ch)
    {
        // check that a different channel hasn`t be selected already.
        if (chanMgr->currFindAndPlayChannel.isSame(ch->info.id))
            chanMgr->playChannel(ch->info);

        if (cfi->keep)
            ch->stayConnected = cfi->keep;
    }

    delete cfi;
    return 0;
}

// -----------------------------------
THREAD_PROC findAndPlayChannelProc(ThreadInfo *thread)
{
    SEH_THREAD(findAndPlayChannelProcMain, findAndPlayChannel);
}

// -----------------------------------
void ChanMgr::findAndPlayChannel(ChanInfo &info, bool keep)
{
    ChanFindInfo *cfi = new ChanFindInfo;
    cfi->info = info;
    cfi->keep = keep;
    cfi->func = findAndPlayChannelProc;

    sys->startThread(cfi);
}

// -----------------------------------
void ChanMgr::playChannel(ChanInfo &info)
{
    char str[128], fname[256], idStr[128];

    sprintf(str, "http://127.0.0.1:%d", servMgr->serverHost.port);
    info.id.toStr(idStr);

    PlayList::TYPE type;

    if ((info.contentType == ChanInfo::T_WMA) || (info.contentType == ChanInfo::T_WMV))
    {
        type = PlayList::T_ASX;
        // WMP seems to have a bug where it doesn`t re-read asx files if they have the same name
        // so we prepend the channel id to make it unique - NOTE: should be deleted afterwards.
        if (servMgr->getModulePath) //JP-EX
        {
            peercastApp->getDirectory();
            sprintf(fname, "%s/%s.asx", servMgr->modulePath, idStr);    
        }else
            sprintf(fname, "%s/%s.asx", peercastApp->getPath(), idStr);
    }else if (info.contentType == ChanInfo::T_OGM)
    {
        type = PlayList::T_RAM;
        if (servMgr->getModulePath) //JP-EX
        {
            peercastApp->getDirectory();
            sprintf(fname, "%s/play.ram", servMgr->modulePath);
        }else
            sprintf(fname, "%s/play.ram", peercastApp->getPath());

    }else
    {
        type = PlayList::T_SCPLS;
        if (servMgr->getModulePath) //JP-EX
        {
            peercastApp->getDirectory();
            sprintf(fname, "%s/play.pls", servMgr->modulePath);
        }else
            sprintf(fname, "%s/play.pls", peercastApp->getPath());
    }

    PlayList *pls = new PlayList(type, 1);
    pls->addChannel(str, info);

    LOG_DEBUG("Writing %s", fname);
    FileStream file;
    file.openWriteReplace(fname);
    pls->write(file);
    file.close();

    LOG_DEBUG("Executing: %s", fname);
    sys->executeFile(fname);
    delete pls;
}

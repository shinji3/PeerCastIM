// ------------------------------------------------
// File : cstream.cpp
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

#include "channel.h"
#include "cstream.h"
#include "pcp.h"
#include "servmgr.h"
#include "version2.h"
#include "critsec.h"

#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif

// -----------------------------------
void ChanPacket::init(ChanPacketv &p)
{
    type = p.type;
    len = p.len;
    if (len > MAX_DATALEN)
        throw StreamException("Packet data too large");
    pos = p.pos;
    sync = p.sync;
    cont = p.cont;
    skip = p.skip;
    priority = p.priority;
    memcpy(data, p.data, len);
}

// -----------------------------------
void ChanPacket::init(TYPE t, const void *p, unsigned int l, unsigned int _pos)
{
    type = t;
    if (l > MAX_DATALEN)
        throw StreamException("Packet data too large");
    len = l;
    memcpy(data, p, len);
    pos = _pos;
}

// -----------------------------------
void ChanPacket::writeRaw(Stream &out)
{
    out.write(data, len);
}

// -----------------------------------
void ChanPacket::writePeercast(Stream &out)
{
    unsigned int tp = 0;
    switch (type)
    {
        case T_HEAD: tp = 'HEAD'; break;
        case T_META: tp = 'META'; break;
        case T_DATA: tp = 'DATA'; break;
    }

    if (type != T_UNKNOWN)
    {
        out.writeTag(tp);
        out.writeShort(len);
        out.writeShort(0);
        out.write(data, len);
    }
}

// -----------------------------------
ChanPacket& ChanPacket::operator=(const ChanPacket& other)
{
    this->type = other.type;
    this->len  = other.len;
    this->pos  = other.pos;
    this->sync = other.sync;
    this->cont = other.cont;
    memcpy(this->data, other.data, this->len);

    return *this;
}

// -----------------------------------
void ChanPacket::readPeercast(Stream &in)
{
    unsigned int tp = in.readTag();

    switch (tp)
    {
        case 'HEAD':    type = T_HEAD; break;
        case 'DATA':    type = T_DATA; break;
        case 'META':    type = T_META; break;
        default:        type = T_UNKNOWN;
    }
    len = in.readShort();
    in.readShort();
    if (len > MAX_DATALEN)
        throw StreamException("Bad ChanPacket");
    in.read(data, len);
}

// -----------------------------------
// (使われていないようだ。)
int ChanPacketBuffer::copyFrom(ChanPacketBuffer &buf, unsigned int reqPos)
{
    lock.on();
    buf.lock.on();

    firstPos = 0;
    lastPos = 0;
    safePos = 0;
    readPos = 0;

    for (unsigned int i = buf.firstPos; i <= buf.lastPos; i++)
    {
        ChanPacketv *src = &buf.packets[i%MAX_PACKETS];
        if (src->type & accept)
        {
            if (src->pos >= reqPos)
            {
                lastPos = writePos;
                packets[writePos++].init(*src);
            }
        }
    }

    buf.lock.off();
    lock.off();
    return lastPos - firstPos;
}

// ------------------------------------------------------------------
// ストリームポジションが spos か、それよりも新しいパケットが見付かれ
// ば pack に代入する。見付かった場合は true, そうでなければ false を
// 返す。
bool ChanPacketBuffer::findPacket(unsigned int spos, ChanPacket &pack)
{
    if (writePos == 0)
        return false;

    lock.on();

    unsigned int bound = packets[0].len * ChanPacketBuffer::MAX_PACKETS * 2; // max packets to wait
    unsigned int fpos = getFirstDataPos();
    unsigned int lpos = getLatestPos();
    if ((spos < fpos && fpos <= lpos && spos != getStreamPosEnd(lastPos)) // --s-----f---l--
        || (spos < fpos && lpos < fpos && spos > lpos + bound)            // -l-------s--f--
        || (spos > lpos && lpos >= fpos && spos - lpos > bound))          // --f---l------s-
        spos = fpos;

    // このループ、lastPos == UINT_MAX の時終了しないのでは？ …4G パ
    // ケットも送らないか。
    for (unsigned int i = firstPos; i <= lastPos; i++)
    {
        //ChanPacket &p = packets[i%MAX_PACKETS];
        ChanPacketv &p = packets[i%MAX_PACKETS];
        if (p.pos >= spos && p.pos - spos <= bound)
        {
            pack.init(p);
            lock.off();
            return true;
        }
    }

    lock.off();
    return false;
}

// ------------------------------------------------------------------
// バッファー内の一番新しいパケットのストリームポジションを返す。まだ
// パケットがない場合は 0 を返す。
unsigned int    ChanPacketBuffer::getLatestPos()
{
    if (!writePos)
        return 0;
    else
        return getStreamPos(lastPos);
}

// ------------------------------------------------------------------
unsigned int    ChanPacketBuffer::getFirstDataPos()
{
    if (!writePos)
        return 0;
    for(unsigned int i=firstPos; i<=lastPos; i++)
    {
        if (packets[i%MAX_PACKETS].type == ChanPacket::T_DATA)
            return packets[i%MAX_PACKETS].pos;
    }
    return 0;
}

// ------------------------------------------------------------------
unsigned int    ChanPacketBuffer::getOldestNonContinuationPos()
{
    if (writePos == 0)
        return 0;

    CriticalSection cs(lock);

    for (int64_t i = firstPos; i <= lastPos; i++)
    {
        ChanPacketv &p = packets[i%MAX_PACKETS];
        if (!p.cont)
            return p.pos;
    }

    return 0;
}

// ------------------------------------------------------------------
// バッファー内の一番古いパケットのストリームポジションを返す。まだパ
// ケットが無い場合は 0 を返す。
unsigned int    ChanPacketBuffer::getOldestPos()
{
    if (!writePos)
        return 0;
    else
        return getStreamPos(firstPos);
}

// -----------------------------------
unsigned int    ChanPacketBuffer::findOldestPos(unsigned int spos)
{
    unsigned int min = getStreamPos(safePos);
    unsigned int max = getStreamPos(lastPos);

    if (min > spos)
        return min;

    if (max < spos)
        return max;

    return spos;
}

// -------------------------------------------------------------------
// パケットインデックス index のパケットのストリームポジションを返す。
unsigned int    ChanPacketBuffer::getStreamPos(unsigned int index)
{
    return packets[index%MAX_PACKETS].pos;
}

// -------------------------------------------------------------------
// パケットインデックス index のパケットの次のパケットのストリームポジ
// ションを計算する。
unsigned int    ChanPacketBuffer::getStreamPosEnd(unsigned int index)
{
    return packets[index%MAX_PACKETS].pos + packets[index%MAX_PACKETS].len;
}

// -----------------------------------
bool ChanPacketBuffer::writePacket(ChanPacket &pack, bool updateReadPos)
{
    if (pack.len)
    {
        if (servMgr->keepDownstreams) {
            unsigned int lpos = getLatestPos();
            unsigned int diff = pack.pos - lpos;
            if (packets[lastPos%MAX_PACKETS].type == ChanPacket::T_HEAD) lpos = 0;
            if (lpos && (diff == 0 || diff > 0xfff00000)) {
                LOG_DEBUG("*   latest pos=%d, pack pos=%d", getLatestPos(), pack.pos);
                lastSkipTime = sys->getTime();
                return false;
            }
        }

        if (willSkip())    // too far behind
        {
            lastSkipTime = sys->getTime();
            return false;
        }

        lock.on();

        pack.sync = writePos;
        packets[writePos%MAX_PACKETS].init(pack);

//        LOG_DEBUG("packet.len = %d",pack.len);

        lastPos = writePos;
        writePos++;

        if (writePos >= MAX_PACKETS)
            firstPos = writePos-MAX_PACKETS;
        else
            firstPos = 0;

        if (writePos >= NUM_SAFEPACKETS)
            safePos = writePos - NUM_SAFEPACKETS;
        else
            safePos = 0;

        if (updateReadPos)
            readPos = writePos;

        lastWriteTime = sys->getTime();

        lock.off();
        return true;
    }

    return false;
}

// -----------------------------------
void    ChanPacketBuffer::readPacket(ChanPacket &pack)
{
    unsigned int tim = sys->getTime();

    if (readPos < firstPos)
        throw StreamException("Read too far behind");

    while (readPos >= writePos)
    {
        sys->sleepIdle();
        if ((sys->getTime() - tim) > 30)
            throw TimeoutException();
    }
    lock.on();
    pack.init(packets[readPos%MAX_PACKETS]);
    readPos++;
    lock.off();
}

// -----------------------------------
void    ChanPacketBuffer::readPacketPri(ChanPacket &pack)
{
    unsigned int tim = sys->getTime();

    if (readPos < firstPos)    
        throw StreamException("Read too far behind");
 
    while (readPos >= writePos)
    {
        sys->sleepIdle();
        if ((sys->getTime() - tim) > 30)
            throw TimeoutException();
    }
    lock.on();
    ChanPacketv *best = &packets[readPos % MAX_PACKETS];
    for (unsigned int i = readPos + 1; i < writePos; i++) {
        if (packets[i % MAX_PACKETS].priority > best->priority)
            best = &packets[i % MAX_PACKETS];
    }
    pack.init(*best);
    best->init(packets[readPos % MAX_PACKETS]);
    readPos++;
    lock.off();
 }

// ------------------------------------------------------------
// バッファーがいっぱいなら true を返す。そうでなければ false。
bool    ChanPacketBuffer::willSkip()
{
    return ((writePos - readPos) >= MAX_PACKETS);
}

// ------------------------------------------
bool ChannelStream::getStatus(Channel *ch, ChanPacket &pack)
{
    unsigned int ctime = sys->getTime();

    if ((ch->isPlaying() == isPlaying)){
        if ((ctime-lastUpdate) < 10){
            return false;
        }

        if ((ctime-lastCheckTime) < 5){
            return false;
        }
        lastCheckTime = ctime;
    }

    ChanHitList *chl = chanMgr->findHitListByID(ch->info.id);

    if (!chl)
        return false;

/*    int newLocalListeners = ch->localListeners();
    int newLocalRelays = ch->localRelays();

    if (
        (
        (numListeners != newLocalListeners)
        || (numRelays != newLocalRelays)
        || (ch->isPlaying() != isPlaying)
        || (servMgr->getFirewall() != fwState)
        || (((ctime - lastUpdate) > chanMgr->hostUpdateInterval) && chanMgr->hostUpdateInterval)
        )
        && ((ctime - lastUpdate) > 10)
       )
    {

        numListeners = newLocalListeners;
        numRelays = newLocalRelays;
        isPlaying = ch->isPlaying();
        fwState = servMgr->getFirewall();
        lastUpdate = ctime;

        ChanHit hit;

        hit.initLocal(ch->localListeners(),ch->localRelays(),ch->info.numSkips,ch->info.getUptime(),isPlaying, ch->isFull(), ch->info.bitrate, ch);
        hit.tracker = ch->isBroadcasting();*/

    int newLocalListeners = ch->localListeners();
    int newLocalRelays = ch->localRelays();

    unsigned int oldp = ch->rawData.getOldestPos();
    unsigned int newp = ch->rawData.getLatestPos();

    ChanHit hit;

//    LOG_DEBUG("isPlaying-------------------------------------- %d %d", ch->isPlaying(), isPlaying);

    hit.initLocal(newLocalListeners,newLocalRelays,ch->info.numSkips,ch->info.getUptime(),ch->isPlaying(), ch->isFull(), ch->info.bitrate, ch, oldp, newp);
    { //JP-MOD
        if(!(ch->info.ppFlags & ServMgr::bcstClap))
            ch->bClap = false;
        hit.initLocal_pp(ch->stealth, ch->bClap ? 1 : 0);
    }
    hit.tracker = ch->isBroadcasting();

    if    (    (((ctime-lastUpdate)>chanMgr->hostUpdateInterval) && chanMgr->hostUpdateInterval)
        ||    (newLocalListeners != numListeners)
        ||    (newLocalRelays != numRelays)
        ||    (ch->isPlaying() != isPlaying)
        ||    (servMgr->getFirewall() != fwState)
        ||    (ch->chDisp.relay != hit.relay)
        ||    (ch->chDisp.relayfull != hit.relayfull)
        ||    (ch->chDisp.chfull != hit.chfull)
        ||    (ch->chDisp.ratefull != hit.ratefull)
        ||    (ch->bClap && ((ctime-lastClapped) > 60)) //JP-MOD    
    ){
        numListeners = newLocalListeners;
        numRelays = newLocalRelays;
        isPlaying = ch->isPlaying();
        fwState = servMgr->getFirewall();
        lastUpdate = ctime;

        if(ch->bClap){ //JP-MOD
            lastClapped = ctime;
            ch->bClap = false;
        }
    
        ch->chDisp = hit;

        if ((numRelays) && ((servMgr->getFirewall() == ServMgr::FW_OFF) && (servMgr->autoRelayKeep!=0))) //JP-EX
            ch->stayConnected = true;

        if ((!numRelays && !numListeners) && (servMgr->autoRelayKeep==2)) //JP-EX
            ch->stayConnected = false;

        MemoryStream pmem(pack.data,sizeof(pack.data));
        AtomStream atom(pmem);

        GnuID noID;
        noID.clear();

        atom.writeParent(PCP_BCST, 10);
            atom.writeChar(PCP_BCST_GROUP, PCP_BCST_GROUP_TRACKERS);
            atom.writeChar(PCP_BCST_HOPS, 0);
            atom.writeChar(PCP_BCST_TTL, 11);
            atom.writeBytes(PCP_BCST_FROM, servMgr->sessionID.id, 16);
            atom.writeInt(PCP_BCST_VERSION, PCP_CLIENT_VERSION);
            atom.writeInt(PCP_BCST_VERSION_VP, PCP_CLIENT_VERSION_VP);
            atom.writeBytes(PCP_BCST_VERSION_EX_PREFIX, PCP_CLIENT_VERSION_EX_PREFIX, 2);
            atom.writeShort(PCP_BCST_VERSION_EX_NUMBER, PCP_CLIENT_VERSION_EX_NUMBER);
            atom.writeBytes(PCP_BCST_CHANID, ch->info.id.id, 16);
            hit.writeAtoms(atom, noID);

        pack.len = pmem.pos;
        pack.type = ChanPacket::T_PCP;
        return true;
    }else
        return false;
}

// ------------------------------------------
void ChannelStream::updateStatus(Channel *ch)
{
    ChanPacket pack;
    if (getStatus(ch, pack))
    {
        if (!ch->isBroadcasting())
        {
            GnuID noID;
            noID.clear();
            int cnt = chanMgr->broadcastPacketUp(pack, ch->info.id, servMgr->sessionID, noID);
            LOG_CHANNEL("Sent channel status update to %d clients", cnt);
        }
    }
}

// -----------------------------------
void ChannelStream::readRaw(Stream &in, Channel *ch)
{
    ChanPacket pack;
    const int readLen = 8192;

    pack.init(ChanPacket::T_DATA, pack.data, readLen, ch->streamPos);
    in.read(pack.data, pack.len);
    ch->newPacket(pack);
    ch->checkReadDelay(pack.len);

    ch->streamPos+=pack.len;
}

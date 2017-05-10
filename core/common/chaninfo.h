#ifndef _CHANINFO_H
#define _CHANINFO_H

// ------------------------------------------------
// File : chaninfo.h
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

#include "sys.h"
#include "xml.h"
#include "atom.h"
#include "http.h"

// ----------------------------------
class TrackInfo
{
public:
    void    clear()
    {
        contact.clear();
        title.clear();
        artist.clear();
        album.clear();
        genre.clear();
    }

    void    convertTo(String::TYPE t)
    {
        contact.convertTo(t);
        title.convertTo(t);
        artist.convertTo(t);
        album.convertTo(t);
        genre.convertTo(t);
    }

    bool    update(const TrackInfo &);

    ::String    contact, title, artist, album, genre;
};

// ----------------------------------
class ChanInfo
{
public:
    enum TYPE
    {
        T_UNKNOWN,

        T_RAW,
        T_MP3,
        T_OGG,
        T_OGM,
        T_MOV,
        T_MPG,
        T_NSV,
        T_FLV,
        T_MKV,
        T_WEBM,
        T_TS,

        T_WMA,
        T_WMV,

        T_PLS,
        T_ASX
    };

    enum PROTOCOL
    {
        SP_UNKNOWN,
        SP_PEERCAST,
        SP_HTTP,
        SP_FILE,
        SP_MMS,
        SP_PCP,
        SP_WMHTTP
    };

    enum STATUS
    {
        S_UNKNOWN,
        S_PLAY
    };

    ChanInfo() { init(); }

    void    init();
    void    init(const char *);
    void    init(const char *, GnuID &, TYPE, int);
    void    init(XML::Node *);
    void    initNameID(const char *);

    void    updateFromXML(XML::Node *);

    void        readTrackXML(XML::Node *);
    void        readServentXML(XML::Node *);
    bool        update(const ChanInfo &);
    XML::Node   *createQueryXML();
    XML::Node   *createChannelXML();
    XML::Node   *createRelayChannelXML();
    XML::Node   *createTrackXML();
    bool        match(XML::Node *);
    bool        match(ChanInfo &);
    bool        matchNameID(ChanInfo &);

    void    writeInfoAtoms(AtomStream &atom);
    void    writeTrackAtoms(AtomStream &atom);

    void    readInfoAtoms(AtomStream &, int);
    void    readTrackAtoms(AtomStream &, int);

    unsigned int        getUptime();
    unsigned int        getAge();
    bool                isActive() { return id.isSet(); }
    bool                isPrivate() { return bcID.getFlags() & 1; }
    const char          *getTypeStr();
    const char          *getTypeExt();
    const char          *getMIMEType();
    static const char   *getTypeStr(TYPE);
    static const char   *getProtocolStr(PROTOCOL);
    static const char   *getTypeExt(TYPE);
    static const char   *getMIMEType(TYPE);
    static TYPE         getTypeFromStr(const char *str);
    static PROTOCOL     getProtocolFromStr(const char *str);
    const char*         getPlayListExt();

    void setContentType(TYPE type);

    ::String        name;
    GnuID           id, bcID;
    int             bitrate;

    // TYPE �̓N���[�Y�h�������ʐ����Ȃ��A�v���g�R����͕�����ł��
    // �Ƃ肷��̂ŁA�璷�ȋC������B

    TYPE            contentType;
    ::String        contentTypeStr; // getTypeStr(contentType) "WMV" �Ȃ�
    ::String        MIMEType;       // MIME �^�C�v
    String          streamExt;      // "." �Ŏn�܂�g���q

    PROTOCOL        srcProtocol;
    unsigned int    lastPlayStart, lastPlayEnd;
    unsigned int    numSkips;
    unsigned int    createdTime;

    STATUS          status;

    TrackInfo       track;
    ::String        desc, genre, url, comment;

    unsigned int    ppFlags; //JP-MOD
};

#endif

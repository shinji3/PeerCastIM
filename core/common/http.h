// ------------------------------------------------
// File : http.h
// Date: 4-apr-2002
// Author: giles
// Desc:
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

#ifndef _HTTP_H
#define _HTTP_H

#include <map>

#include "stream.h"
#include "str.h"

// -------------------------------------
class HTTPException : public StreamException
{
public:
    HTTPException(const char *m, int c) : StreamException(m) { code=c; }
    int code;
};

// --------------------------------------------
#define HTTP_SC_OK           "HTTP/1.0 200 OK"
#define HTTP_SC_NOTFOUND     "HTTP/1.0 404 Not Found"
#define HTTP_SC_UNAVAILABLE  "HTTP/1.0 503 Service Unavailable"
#define HTTP_SC_UNAUTHORIZED "HTTP/1.0 401 Unauthorized"
#define HTTP_SC_FOUND        "HTTP/1.0 302 Found"
#define HTTP_SC_BADREQUEST   "HTTP/1.0 400 Bad Request"
#define HTTP_SC_FORBIDDEN    "HTTP/1.0 403 Forbidden"
#define HTTP_SC_SWITCH       "HTTP/1.0 101 Switch protocols"

#define HTTP_PROTO1          "HTTP/1."

#define HTTP_HS_SERVER       "Server:"
#define HTTP_HS_AGENT        "User-Agent:"
#define HTTP_HS_CONTENT      "Content-Type:"
#define HTTP_HS_CACHE        "Cache-Control:"
#define HTTP_HS_CONNECTION   "Connection:"
#define HTTP_HS_SETCOOKIE    "Set-Cookie:"
#define HTTP_HS_COOKIE       "Cookie:"
#define HTTP_HS_HOST         "Host:"
#define HTTP_HS_ACCEPT       "Accept:"
#define HTTP_HS_LENGTH       "Content-Length:"

#define MIME_MP3             "audio/mpeg"
#define MIME_XMP3            "audio/x-mpeg"
#define MIME_OGG             "application/ogg"
#define MIME_XOGG            "application/x-ogg"
#define MIME_MOV             "video/quicktime"
#define MIME_MPG             "video/mpeg"
#define MIME_NSV             "video/nsv"
#define MIME_ASF             "video/x-ms-asf"
#define MIME_ASX             "video/x-ms-asf"
// same as ASF
#define MIME_MMS             "application/x-mms-framed"

#define MIME_RAM             "audio/x-pn-realaudio"

#define MIME_WMA             "audio/x-ms-wma"
#define MIME_WMV             "video/x-ms-wmv"
#define MIME_FLV             "video/x-flv"
#define MIME_MKV             "video/x-matroska"
#define MIME_WEBM            "video/webm"
#define MIME_TS              "video/MP2T"

#define MIME_HTML            "text/html"
#define MIME_XML             "text/xml"
#define MIME_CSS             "text/css"
#define MIME_TEXT            "text/plain"
#define MIME_PLS             "audio/mpegurl"
#define MIME_XPLS            "audio/x-mpegurl"
#define MIME_XSCPLS          "audio/x-scpls"
#define MIME_SDP             "application/sdp"
#define MIME_M3U             "audio/m3u"
#define MIME_MPEGURL         "audio/mpegurl"
#define MIME_XM3U            "audio/x-mpegurl"
#define MIME_XPEERCAST       "application/x-peercast"
#define MIME_XPCP            "application/x-peercast-pcp"
#define MIME_RAW             "application/binary"
#define MIME_JPEG            "image/jpeg"
#define MIME_GIF             "image/gif"
#define MIME_PNG             "image/png"

// --------------------------------------------
class Cookie
{
public:
    Cookie()
    {
        clear();
    }

    void    clear()
    {
        time = 0;
        ip = 0;
        id[0]=0;
    }

    void    set(const char *i, unsigned int nip)
    {
        strncpy(id, i, sizeof(id)-1);
        id[sizeof(id)-1]=0;
        ip = nip;
    }
    bool    compare(Cookie &c)
    {
        if (c.ip == ip)
            if (strcmp(c.id, id)==0)
                return true;

        return false;
    }

    void    logDebug(const char *, int);

    unsigned int    ip;
    char            id[64];
    unsigned int    time;
};

// --------------------------------------------
class CookieList
{
public:
    enum {
        MAX_COOKIES = 32
    };

    void    init();
    bool    add(Cookie &);
    void    remove(Cookie &);
    bool    contains(Cookie &);

    Cookie  list[MAX_COOKIES];
    bool    neverExpire;
};

// --------------------------------------------
class HTTPRequest
{
public:
    HTTPRequest(const std::string& aMethod, const std::string& aUrl, const std::string& aProtocolVersion,
        std::map<std::string,std::string>& aHeaders)
        : method(aMethod)
        , url(aUrl)
        , protocolVersion(aProtocolVersion)
        , headers(aHeaders)
    {
        auto vec = str::split(url, "?");
        if (vec.size() >= 2)
        {
            path = vec[0];
            queryString = vec[1];
        }else
            path = url;
    }

    std::string getHeader(const std::string& name) const
    {
        auto it = headers.find(str::upcase(name));
        if (it == headers.end())
            return "";
        else
            return it->second;
    }

    std::string method;
    std::string url;
    std::string path;
    std::string queryString;
    std::string protocolVersion;
    std::map<std::string,std::string> headers;
};

// --------------------------------------------
class HTTPResponse
{
public:

    HTTPResponse(int aStatusCode, const std::map<std::string,std::string>& aHeaders)
        : statusCode(aStatusCode)
        , headers(aHeaders)
        , stream(NULL)
    {
    }

    static HTTPResponse ok(const std::map<std::string,std::string>& aHeaders, const std::string& body)
    {
        HTTPResponse res(200, aHeaders);
        res.body = body;
        return res;
    }

    static HTTPResponse notFound()
    {
        HTTPResponse res(400, {{"Content-Type", "text/html"}});
        res.body = "File not found";
        return res;
    }

    static HTTPResponse redirectTo(const std::string& url)
    {
        HTTPResponse res(302, {{"Location",url}});
        return res;
    }

    std::string body;
    int statusCode;
    std::map<std::string,std::string> headers;
    Stream* stream;
};

// --------------------------------------------
class HTTP : public IndirectStream
{
public:
    HTTP(Stream &s)
    {
        init(&s);
    }

    void    initRequest(const char *r)
    {
        strcpy(cmdLine,r);
    }
    void    readRequest();
    bool    isRequest(const char *);

    int     readResponse();
    bool    checkResponse(int);

    bool    nextHeader();
    bool    isHeader(const char *);
    char    *getArgStr();
    int     getArgInt();

    void    getAuthUserPass(char *, char *, size_t, size_t);

    char    cmdLine[8192], *arg;
};

#endif

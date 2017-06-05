// ------------------------------------------------
// File : servhs.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      Servent handshaking, TODO: should be in its own class
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

#include <stdlib.h>
#include "servent.h"
#include "servmgr.h"
#include "html.h"
#include "stats.h"
#include "peercast.h"
#include "pcp.h"
#include "version2.h"
#include "jrpc.h"
#include "playlist.h"
#include "dechunker.h"
#include "matroska.h"
#include "str.h"
#include "cgi.h"
#include "template.h"
#include "public.h"
#include "md5.h"

#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif

using namespace std;

// -----------------------------------
static void termArgs(char *str)
{
    if (str)
    {
        size_t slen = strlen(str);
        for (size_t i=0; i<slen; i++)
            if (str[i]=='&') str[i] = 0;
    }
}

// -----------------------------------
char *nextCGIarg(char *cp, char *cmd, char *arg)
{
    if (!*cp)
        return NULL;

    int cnt=0;

    // fetch command
    while (*cp)
    {
        char c = *cp++;
        if (c == '=')
            break;
        else
            *cmd++ = c;

        cnt++;
        if (cnt >= (MAX_CGI_LEN-1))
            break;
    }
    *cmd = 0;

    cnt=0;
    // fetch arg
    while (*cp)
    {
        char c = *cp++;
        if (c == '&')
            break;
        else
            *arg++ = c;

        cnt++;
        if (cnt >= (MAX_CGI_LEN-1))
            break;
    }
    *arg = 0;

    return cp;
}

// -----------------------------------
bool getCGIargBOOL(char *a)
{
    return (strcmp(a, "1") == 0);
}

// -----------------------------------
int getCGIargINT(char *a)
{
    return atoi(a);
}

// -----------------------------------
void Servent::handshakeJRPC(HTTP &http)
{
    int content_length = -1;

    string lenstr = http.headers.get("Content-Length");
    if (!lenstr.empty())
        content_length = atoi(lenstr.c_str());

    if (content_length == -1)
        throw HTTPException("HTTP/1.0 411 Length required", 411);

    if (content_length == 0)
        throw HTTPException(HTTP_SC_BADREQUEST, 400);

    unique_ptr<char> body(new char[content_length + 1]);
    try {
        http.stream->read(body.get(), content_length);
        body.get()[content_length] = '\0';
    }catch (SockException&)
    {
        // body too short
        throw HTTPException(HTTP_SC_BADREQUEST, 400);
    }

    JrpcApi api;
    std::string response = api.call(body.get());

    http.writeLine(HTTP_SC_OK);
    http.writeLineF("%s %s", HTTP_HS_SERVER, PCX_AGENT);
    http.writeLineF("%s %zu", HTTP_HS_LENGTH, response.size());
    http.writeLineF("%s %s", HTTP_HS_CONTENT, "application/json");
    http.writeLine("");

    http.write(response.c_str(), static_cast<int>(response.size()));
}

// -----------------------------------
bool Servent::hasValidAuthToken(const std::string& requestFilename)
{
    auto vec = str::split(requestFilename, "?");

    if (vec.size() != 2)
        return false;

    cgi::Query query(vec[1]);

    auto token = query.get("auth");
    auto chanid = str::upcase(vec[0].substr(0, 32));
    auto validToken = chanMgr->authToken(chanid);

    if (validToken == token)
    {
        return true;
    }

    return false;
}

// -----------------------------------
#include <unistd.h>
#include "env.h"

#include <sys/types.h>
#include <sys/wait.h>

#include "regexp.h"

void Servent::invokeCGIScript(HTTP &http, const char* fn)
{
    HTTPRequest req;
    try {
        req = http.getRequest();
    } catch (GeneralException& e) // request not ready
    {
        throw HTTPException(HTTP_SC_BADREQUEST, 400);
    }
    FileSystemMapper fs("/cgi-bin", (std::string) peercastApp->getPath() + "cgi-bin");
    std::string filePath = fs.toLocalFilePath(req.path);
    Environment env;

    env.set("SCRIPT_NAME", req.path);
    env.set("SCRIPT_FILENAME", filePath);
    env.set("GATEWAY_INTERFACE", "CGI/1.1");
    env.set("DOCUMENT_ROOT", peercastApp->getPath());
    env.set("QUERY_STRING", req.queryString);
    env.set("REQUEST_METHOD", "GET");
    // env.set("REQUEST_SCHEME", "");
    env.set("REQUEST_URI", req.url);
    // env.set("REMOTE_ADDR", "");
    // env.set("REMOTE_PORT", "");
    // env.set("SERVER_ADDR", "");
    // env.set("SERVER_ADMIN", "");
    // env.set("SERVER_NAME", "");
    // env.set("SERVER_PORT", "");
    env.set("SERVER_PROTOCOL", "HTTP/1.0");
    env.set("SERVER_SOFTWARE", PCX_AGENT);
    env.set("PATH", getenv("PATH"));

    if (filePath.empty())
        throw HTTPException(HTTP_SC_NOTFOUND, 404);

    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        char buf[1024];
        strerror_r(errno, buf, sizeof(buf));
        LOG_ERROR("pipe: %s", buf);

        throw HTTPException(HTTP_SC_SERVERERROR, 500);
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        // �q�v���Z�X�B

        // �p�C�v�̓ǂݏo���������B
        close(pipefd[0]);

        // �������ݑ���W���o�͂Ƃ��ĕ�������B
        close(1);
        dup(pipefd[1]);

        close(pipefd[1]);

        int r = execle(filePath.c_str(),
                       filePath.c_str(), NULL,
                       env.env());
        if (r == -1)
        {
            // char buf[1024];
            // strerror_r(errno, buf, sizeof(buf));

            char *buf = strerror(errno);
            LOG_ERROR("execle: %s: %s", filePath.c_str(), buf);

            printf("Status: 500 Internal Server Error\n");
            printf("Content-Type: text/plain\n\n");

            printf("execle: %s: %s", filePath.c_str(), buf);
            fflush(stdout);
            _exit(0);
        }
    }else
    {
        // �e�v���Z�X�B

        // �������ݑ������B
        close(pipefd[1]);

        FileStream stream;
        stream.openReadOnly(pipefd[0]);

        HTTPHeaders headers;
        int statusCode = 200;
        try {
            Regexp headerPattern("\\A([A-Za-z\\-]+):\\s*(.*)\\z");
            std::string line;
            while ((line = stream.readLine()) != "")
            {
                auto caps = headerPattern.exec(line);
                if (caps.size() == 0)
                {
                    LOG_ERROR("Invalid header: \"%s\"", line.c_str());
                    continue;
                }
                if (str::capitalize(caps[1]) == "Status")
                    statusCode = atoi(caps[2].c_str());
                else
                    headers.set(caps[1], caps[2]);
            }
            if (headers.get("Location") != "")
                statusCode = 302; // Found
        } catch (StreamException&)
        {
            throw HTTPException(HTTP_SC_SERVERERROR, 500);
        }

        std::string body;
        try
        {
            while (true)
                body += stream.readChar();
        } catch (StreamException&) {}

        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
        {
            LOG_DEBUG("child process (PID %d) exited normally (status %d)", pid, WEXITSTATUS(status));
            if (WEXITSTATUS(status) != 0)
                throw HTTPException(HTTP_SC_SERVERERROR, 500);
        }
        else
        {
            LOG_ERROR("child process (PID %d) terminated abnormally", pid);
            throw HTTPException(HTTP_SC_SERVERERROR, 500);
        }

        HTTPResponse res(statusCode, headers);
        res.body = body;

        http.send(res);
    }
}

// -----------------------------------
void Servent::handshakeGET(HTTP &http)
{
    char *fn = http.cmdLine + 4;

    char *pt = strstr(fn, HTTP_PROTO1);
    if (pt)
        pt[-1] = 0;

    if (strncmp(fn, "/admin?", 7) == 0)
    {
        // �t�H�[�����e�p�G���h�|�C���g

        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        LOG_DEBUG("Admin client");
        handshakeCMD(fn+7);
    }else if (strncmp(fn, "/admin/?", 8) == 0)
    {
        // ��ɓ���

        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        LOG_DEBUG("Admin client");
        handshakeCMD(fn+8);
    }else if (strncmp(fn, "/http/", 6) == 0)
    {
        // peercast.org �ւ̃v���L�V�ڑ�

        String dirName = fn+6;

        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        if (!handshakeAuth(http, fn, false))
            throw HTTPException(HTTP_SC_UNAUTHORIZED, 401);

        handshakeRemoteFile(dirName);
    }else if (strcmp(fn, "/html/index.html") == 0)
    {
        // PeerCastStation �� "/" �� "/html/index.html" �� 301 Moved
        // �Ń��_�C���N�g����̂ŁA�u���E�U�ɂ���Ă͖������ɃL���b�V�������B
        // "/" �ɍă��_�C���N�g���ăL���b�V���𖳌�������B

        http.readHeaders();
        http.writeLine(HTTP_SC_FOUND);
        http.writeLineF("Location: /");
        http.writeLine("");
    }else if (strncmp(fn, "/html/", 6) == 0)
    {
        // HTML UI

        String dirName = fn+1;

        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        if (handshakeAuth(http, fn, true))
            handshakeLocalFile(dirName);
    }else if (strncmp(fn, "/admin.cgi", 10) == 0)
    {
        // ShoutCast �g���b�N���X�V�p�G���h�|�C���g

        if (!isAllowed(ALLOW_BROADCAST))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        const char *pwdArg = getCGIarg(fn, "pass=");
        const char *songArg = getCGIarg(fn, "song=");
        const char *mountArg = getCGIarg(fn, "mount=");
        const char *urlArg = getCGIarg(fn, "url=");

        if (pwdArg && songArg)
        {
            size_t slen = strlen(fn);
            for (size_t i=0; i<slen; i++)
                if (fn[i]=='&') fn[i] = 0;

            Channel *c=chanMgr->channel;
            while (c)
            {
                if ((c->status == Channel::S_BROADCASTING) &&
                    (c->info.contentType == ChanInfo::T_MP3) )
                {
                    // if we have a mount point then check for it, otherwise update all channels.

                    bool match=true;

                    if (mountArg)
                        match = strcmp(c->mount, mountArg) == 0;

                    if (match)
                    {
                        ChanInfo newInfo = c->info;
                        newInfo.track.title.set(songArg, String::T_ESC);
                        newInfo.track.title.convertTo(String::T_UNICODE);

                        if (urlArg)
                            if (urlArg[0])
                                newInfo.track.contact.set(urlArg, String::T_ESC);
                        LOG_CHANNEL("Channel Shoutcast update: %s", songArg);
                        c->updateInfo(newInfo);
                    }
                }
                c=c->next;
            }
        }
    }else if (strncmp(fn, "/pls/", 5) == 0)
    {
        // �v���C���X�g

        if (!sock->host.isLocalhost())
            if (!isAllowed(ALLOW_DIRECT) || !isFiltered(ServFilter::F_DIRECT))
                throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        ChanInfo info;
        if (servMgr->getChannel(fn+5, info, isPrivate() || hasValidAuthToken(fn+5)))
            handshakePLS(info, false);
        else
            throw HTTPException(HTTP_SC_NOTFOUND, 404);
    }else if (strncmp(fn, "/stream/", 8) == 0)
    {
        // �X�g���[��

        if (!sock->host.isLocalhost())
            if (!isAllowed(ALLOW_DIRECT) || !isFiltered(ServFilter::F_DIRECT))
                throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        triggerChannel(fn+8, ChanInfo::SP_HTTP, isPrivate() || hasValidAuthToken(fn+8));
    }else if (strncmp(fn, "/channel/", 9) == 0)
    {
        if (!sock->host.isLocalhost())
            if (!isAllowed(ALLOW_NETWORK) || !isFiltered(ServFilter::F_NETWORK))
                throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        triggerChannel(fn+9, ChanInfo::SP_PCP, false);
    }else if (strcmp(fn, "/api/1") == 0)
    {
        // JSON RPC �o�[�W�������擾�p

        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        if (handshakeHTTPBasicAuth(http))
        {
            JrpcApi api;
            std::string response = api.getVersionInfo(nlohmann::json::array_t()).dump();

            http.writeLine(HTTP_SC_OK);
            http.writeLineF("%s %zu", HTTP_HS_LENGTH, response.size());
            http.writeLine("");
            http.writeString(response.c_str());
        }
    }else if (strcmp(fn, "/public")== 0 ||
              strncmp(fn, "/public/", strlen("/public/"))==0)
    {
        // ���J�f�B���N�g��

        http.readHeaders();

        if (!servMgr->publicDirectoryEnabled)
        {
            throw HTTPException(HTTP_SC_FORBIDDEN, 403);
        }

        try
        {
            PublicController controller(peercastApp->getPath() + std::string("public"));
            http.send(controller(http.getRequest(), *sock, sock->host));
        } catch (GeneralException& e)
        {
            LOG_ERROR("Error: %s", e.msg);
            throw HTTPException(HTTP_SC_SERVERERROR, 500);
        }
    }else if (str::is_prefix_of("/assets/", fn))
    {
        // html �� public �̋��L�A�Z�b�g�B

        http.readHeaders();
        try
        {
            AssetsController controller(peercastApp->getPath() + std::string("assets"));
            http.send(controller(http.getRequest(), *sock, sock->host));
        } catch (GeneralException& e)
        {
            LOG_ERROR("Error: %s", e.msg);
            throw HTTPException(HTTP_SC_SERVERERROR, 500);
        }
    }else if (str::is_prefix_of("/cgi-bin/", fn))
    {
        // CGI �X�N���v�g�̎��s

        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        if (handshakeAuth(http, fn, true))
            invokeCGIScript(http, fn);
    }else
    {
        // GET �}�b�`�Ȃ�

        http.readHeaders();
        http.writeLine(HTTP_SC_FOUND);
        http.writeLineF("Location: /%s/index.html", servMgr->htmlPath);
        http.writeLine("");
    }
}

// -----------------------------------
void Servent::handshakePOST(HTTP &http)
{
    LOG_DEBUG("cmdLine: %s", http.cmdLine);

    auto vec = str::split(http.cmdLine, " ");
    if (vec.size() != 3)
        throw HTTPException(HTTP_SC_BADREQUEST, 400);

    std::string args;
    auto vec2 = str::split(vec[1], "?");

    if (vec2.size() >= 2)
        args = vec2[1];

    std::string path = vec2[0];

    if (strcmp(path.c_str(), "/api/1") == 0)
    {
        // JSON API

        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        if (handshakeHTTPBasicAuth(http))
            handshakeJRPC(http);
    }else if (strcmp(path.c_str(), "/") == 0)
    {
        // HTTP Push

        if (!isAllowed(ALLOW_BROADCAST))
            throw HTTPException(HTTP_SC_FORBIDDEN, 403);

        if (!isPrivate())
            throw HTTPException(HTTP_SC_FORBIDDEN, 403);

        handshakeHTTPPush(args);
    }else
    {
        http.readHeaders();
        auto contentType = http.headers.get("Content-Type");
        if (contentType == "application/x-wms-pushsetup")
        {
            // WMHTTP

            if (!isAllowed(ALLOW_BROADCAST))
                throw HTTPException(HTTP_SC_FORBIDDEN, 403);

            if (!isPrivate())
                throw HTTPException(HTTP_SC_FORBIDDEN, 403);

            handshakeWMHTTPPush(http, path);
        }else
        {
            // POST �}�b�`�Ȃ�

            throw HTTPException(HTTP_SC_BADREQUEST, 400);
        }
    }
}

// -----------------------------------
void Servent::handshakeGIV(const char *requestLine)
{
    HTTP(*sock).readHeaders();

    if (!isAllowed(ALLOW_NETWORK))
        throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

    GnuID id;

    auto *idstr = strstr(requestLine, "/");
    if (idstr)
        id.fromStr(idstr+1);

    char ipstr[64];
    sock->host.toStr(ipstr);

    if (id.isSet())
    {
        // at the moment we don`t really care where the GIV came from, so just give to chan. no. if its waiting.
        Channel *ch = chanMgr->findChannelByID(id);

        if (!ch)
            throw HTTPException(HTTP_SC_NOTFOUND, 404);

        if (!ch->acceptGIV(sock))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        LOG_DEBUG("Accepted GIV channel %s from: %s", idstr, ipstr);
        sock=NULL;                  // release this servent but dont close socket.
    }else
    {
        if (!servMgr->acceptGIV(sock))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        LOG_DEBUG("Accepted GIV PCP from: %s", ipstr);
        sock=NULL;                  // release this servent but dont close socket.
    }
}

// -----------------------------------
void Servent::handshakeSOURCE(char * in, bool isHTTP)
{
    if (!isAllowed(ALLOW_BROADCAST))
        throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

    char *mount = NULL;

    char *ps;
    if (ps=strstr(in, "ICE/1.0"))
    {
        mount = in+7;
        *ps = 0;
        LOG_DEBUG("ICE 1.0 client to %s", mount?mount:"unknown");
    }else{
        mount = in+strlen(in);
        while (*--mount)
            if (*mount == '/')
            {
                mount[-1] = 0; // password preceeds
                break;
            }
        loginPassword.set(in+7);

        LOG_DEBUG("ICY client: %s %s", loginPassword.cstr(), mount?mount:"unknown");
    }

    if (mount)
        loginMount.set(mount);

    handshakeICY(Channel::SRC_ICECAST, isHTTP);
    sock = NULL;    // socket is taken over by channel, so don`t close it
}

// -----------------------------------
void Servent::handshakeHTTP(HTTP &http, bool isHTTP)
{
    if (http.isRequest("GET /"))
    {
        handshakeGET(http);
    }else if (http.isRequest("POST /"))
    {
        handshakePOST(http);
    }else if (http.isRequest("GIV"))
    {
        // Push �����[

        handshakeGIV(http.cmdLine);
    }else if (http.isRequest(PCX_PCP_CONNECT)) // "pcp"
    {
        // CIN

        if (!isAllowed(ALLOW_NETWORK) || !isFiltered(ServFilter::F_NETWORK))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        processIncomingPCP(true);
    }else if (http.isRequest("PEERCAST CONNECT"))
    {
        if (!isAllowed(ALLOW_NETWORK) || !isFiltered(ServFilter::F_NETWORK))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        LOG_DEBUG("PEERCAST client");
        processServent();
    }else if (http.isRequest("SOURCE"))
    {
        // Icecast ����

        handshakeSOURCE(http.cmdLine, isHTTP);
    }else if (http.isRequest(servMgr->password)) // FIXME: check for empty password!
    {
        // ShoutCast broadcast

        if (!isAllowed(ALLOW_BROADCAST))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        loginPassword.set(servMgr->password);   // pwd already checked

        sock->writeLine("OK2");
        sock->writeLine("icy-caps:11");
        sock->writeLine("");
        LOG_DEBUG("ShoutCast client");

        handshakeICY(Channel::SRC_SHOUTCAST, isHTTP);
        sock = NULL;    // socket is taken over by channel, so don`t close it
    }else
    {
        // ���N�G�X�g���ߎ��s

        throw HTTPException(HTTP_SC_BADREQUEST, 400);
    }
}

// -----------------------------------
void Servent::handshakeIncoming()
{
    setStatus(S_HANDSHAKE);

    char buf[8192];

    if ((size_t)sock->readLine(buf, sizeof(buf)) >= sizeof(buf)-1)
    {
        throw HTTPException(HTTP_SC_URITOOLONG, 414);
    }

    bool isHTTP = (stristr(buf, HTTP_PROTO1) != NULL);

    if (isHTTP)
        LOG_DEBUG("HTTP from %s '%s'", sock->host.str().c_str(), buf);
    else
        LOG_DEBUG("Connect from %s '%s'", sock->host.str().c_str(), buf);

    HTTP http(*sock);
    http.initRequest(buf);
    handshakeHTTP(http, isHTTP);
}

// -----------------------------------
// �����[�ڑ��A���邢�̓_�C���N�g�ڑ��� str �Ŏw�肳�ꂽ�`�����l���̃X
// �g���[���𗬂��Brelay �� true �Ȃ�΁A�`�����l���������������M��
// �łȂ��Ă��A�`�����l������M���̏�Ԃɂ��悤�Ƃ���B
void Servent::triggerChannel(char *str, ChanInfo::PROTOCOL proto, bool relay)
{
    ChanInfo info;

    servMgr->getChannel(str, info, relay);

    if (proto == ChanInfo::SP_PCP)
        type = T_RELAY;
    else
        type = T_DIRECT;

    outputProtocol = proto;

    processStream(false, info);
}

// -----------------------------------
void writePLSHeader(Stream &s, PlayList::TYPE type)
{
    s.writeLine(HTTP_SC_OK);
    s.writeLineF("%s %s", HTTP_HS_SERVER, PCX_AGENT);

    const char *content;
    switch(type)
    {
        case PlayList::T_PLS:
            content = MIME_XM3U;
            break;
        case PlayList::T_ASX:
            content = MIME_ASX;
            break;
        case PlayList::T_RAM:
            content = MIME_RAM;
            break;
        default:
            content = MIME_TEXT;
            break;
    }
    s.writeLineF("%s %s", HTTP_HS_CONTENT, content);
    s.writeLine("Content-Disposition: inline");
    s.writeLine("Cache-Control: private" );
    s.writeLineF("%s %s", HTTP_HS_CONNECTION, "close");

    s.writeLine("");
}

// -----------------------------------
void Servent::handshakePLS(ChanInfo &info, bool doneHandshake)
{
    char url[256];
    char in[128];

    if (!doneHandshake)
        while (sock->readLine(in, 128));

    if (getLocalURL(url))
    {
        PlayList::TYPE type = PlayList::getPlayListType(info.contentType);

        writePLSHeader(*sock, type);

        PlayList pls(type, 1);
        pls.addChannel(url, info);
        pls.write(*sock);
    }
}

// -----------------------------------
void Servent::handshakePLS(ChanHitList **cl, int num, bool doneHandshake)
{
    char url[256];
    char in[128];

    if (!doneHandshake)
        while (sock->readLine(in, 128));

    if (getLocalURL(url))
    {
        writePLSHeader(*sock, PlayList::T_SCPLS);

        PlayList pls(PlayList::T_SCPLS, num);

        for (int i=0; i<num; i++)
            pls.addChannel(url, cl[i]->info);

        pls.write(*sock);
    }
}

// -----------------------------------
bool Servent::getLocalURL(char *str)
{
    if (!sock)
        throw StreamException("Not connected");

    char ipStr[64];

    Host h;

    if (sock->host.localIP())
        h = sock->getLocalHost();
    else
        h = servMgr->serverHost;

    h.port = servMgr->serverHost.port;

    h.toStr(ipStr);

    sprintf_s(str, 29, "http://%s", ipStr);
    return true;
}

// -----------------------------------
bool Servent::handshakeHTTPBasicAuth(HTTP &http)
{
    char user[64] = "", pass[64] = "";

    while (http.nextHeader())
    {
        char *arg = http.getArgStr();
        if (!arg)
            continue;

        if (http.isHeader("Authorization"))
            http.getAuthUserPass(user, pass, sizeof(user), sizeof(pass));
    }

    if (sock->host.isLocalhost())
        return true;

    if (strlen(servMgr->password) != 0 && strcmp(pass, servMgr->password) == 0)
        return true;

    http.writeLine(HTTP_SC_UNAUTHORIZED);
    http.writeLine("WWW-Authenticate: Basic realm=\"PeerCast Admin\"");
    http.writeLine("");
    return false;
}

// -----------------------------------
bool Servent::handshakeAuth(HTTP &http, const char *args, bool local)
{
    char user[64], pass[64];
    user[0] = pass[0] = 0;

    const char *pwd  = getCGIarg(args, "pass=");

    if ((pwd) && strlen(servMgr->password))
    {
        String tmp = pwd;
        char *as = strstr(tmp.cstr(), "&");
        if (as) *as = 0;
        if (strcmp(tmp, servMgr->password) == 0)
        {
            http.readHeaders();
            return true;
        }
    }

    Cookie gotCookie;
    cookie.clear();

    while (http.nextHeader())
    {
        char *arg = http.getArgStr();
        if (!arg)
            continue;

        switch (servMgr->authType)
        {
            case ServMgr::AUTH_HTTPBASIC:
                if (http.isHeader("Authorization"))
                    http.getAuthUserPass(user, pass, sizeof(user), sizeof(pass));
                break;
            case ServMgr::AUTH_COOKIE:
                if (http.isHeader("Cookie"))
                {
                    LOG_DEBUG("Got cookie: %s", arg);
                    char *idp=arg;
                    while ((idp = strstr(idp, "id=")))
                    {
                        idp+=3;
                        gotCookie.set(idp, sock->host.ip);
                        if (servMgr->cookieList.contains(gotCookie))
                        {
                            LOG_DEBUG("Cookie found");
                            cookie = gotCookie;
                            break;
                        }
                    }
                }
                break;
        }
    }

    if (sock->host.isLocalhost())
        return true;

    switch (servMgr->authType)
    {
        case ServMgr::AUTH_HTTPBASIC:
            if ((strcmp(pass, servMgr->password) == 0) && strlen(servMgr->password))
                return true;
            break;
        case ServMgr::AUTH_COOKIE:
            if (servMgr->cookieList.contains(cookie))
                return true;
            break;
    }

    if (servMgr->authType == ServMgr::AUTH_HTTPBASIC)
    {
        http.writeLine(HTTP_SC_UNAUTHORIZED);
        http.writeLine("WWW-Authenticate: Basic realm=\"PeerCast Admin\"");
        http.writeLine("");
    }else if (servMgr->authType == ServMgr::AUTH_COOKIE)
    {
        String file = servMgr->htmlPath;
        file.append("/login.html");
        if (local)
        {
            if (http.headers.get("X-Requested-With") == "XMLHttpRequest")
                throw HTTPException(HTTP_SC_FORBIDDEN, 403);
            else
                handshakeLocalFile(file);
        }else
            handshakeRemoteFile(file);
    }

    return false;
}

// -----------------------------------
void Servent::CMD_redirect(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    const char *j = getCGIarg(cmd, "url=");

    if (j)
    {
        http.writeLine(HTTP_SC_OK);
        http.writeLineF("%s %s", HTTP_HS_SERVER, PCX_AGENT);
        http.writeLineF("%s %s", HTTP_HS_CONTENT, "text/html");
        http.writeLine("");

        termArgs(cmd);
        String url;
        url.set(j, String::T_ESC);
        url.convertTo(String::T_ASCII);

        if (!url.contains("http://"))
            url.prepend("http://");

        html.setRefreshURL(url.cstr());
        html.startHTML();
            html.addHead();
            html.startBody();
                html.startTagEnd("h3", "Please wait...");
            html.end();
        html.end();
    } else {
        throw HTTPException(HTTP_SC_BADREQUEST, 400);
    }
}

void Servent::CMD_viewxml(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    handshakeXML();
}

void Servent::CMD_clearlog(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    sys->logBuf->clear();
    jumpStr.sprintf("/%s/viewlog.html", servMgr->htmlPath);
}

void Servent::CMD_save(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    peercastInst->saveSettings();

    jumpStr.sprintf(jumpStr, "/%s/settings.html", servMgr->htmlPath);
}

void Servent::CMD_reg(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    char idstr[128];

    chanMgr->broadcastID.toStr(idstr);
    jumpStr.sprintf("http://www.peercast.org/register/?id=%s", idstr);
}

void Servent::CMD_edit_bcid(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    char arg[MAX_CGI_LEN];
    char curr[MAX_CGI_LEN];

    char *cp = cmd;
    GnuID id;
    BCID *bcid;

    while (cp=nextCGIarg(cp, curr, arg))
    {
        if (strcmp(curr, "id") == 0)
            id.fromStr(arg);
        else if (strcmp(curr, "del") == 0)
            servMgr->removeValidBCID(id);
        else if (strcmp(curr, "valid") == 0)
        {
            bcid = servMgr->findValidBCID(id);
            if (bcid)
                bcid->valid = getCGIargBOOL(arg);
        }
    }

    peercastInst->saveSettings();
    jumpStr.sprintf("/%s/bcid.html", servMgr->htmlPath);
}

void Servent::CMD_add_bcid(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    char arg[MAX_CGI_LEN];
    char curr[MAX_CGI_LEN];

    BCID *bcid = new BCID();

    char *cp = cmd;
    bool result=false;
    while (cp=nextCGIarg(cp, curr, arg))
    {
        if (strcmp(curr, "id") == 0)
            bcid->id.fromStr(arg);
        else if (strcmp(curr, "name") == 0)
            bcid->name.set(arg);
        else if (strcmp(curr, "email") == 0)
            bcid->email.set(arg);
        else if (strcmp(curr, "url") == 0)
            bcid->url.set(arg);
        else if (strcmp(curr, "valid") == 0)
            bcid->valid = getCGIargBOOL(arg);
        else if (strcmp(curr, "result") == 0)
            result = true;
    }

    LOG_DEBUG("Adding BCID : %s", bcid->name.cstr());
    servMgr->addValidBCID(bcid);
    peercastInst->saveSettings();
    if (result)
    {
        http.writeLine(HTTP_SC_OK);
        http.writeLine("");
        http.writeString("OK");
    }else
    {
        jumpStr.sprintf("/%s/bcid.html", servMgr->htmlPath);
    }
}

void Servent::CMD_apply(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    servMgr->numFilters = 0;
    ServFilter *currFilter = servMgr->filters;
    servMgr->channelDirectory.clearFeeds();
    servMgr->publicDirectoryEnabled = false;

    bool brRoot = false;
    bool getUpd = false;
    int showLog = 0;
    int allowServer1 = 0;
    int allowServer2 = 0;
    int newPort = servMgr->serverHost.port;

    char arg[MAX_CGI_LEN];
    char curr[MAX_CGI_LEN];
    char *cp = cmd;
    while (cp=nextCGIarg(cp, curr, arg))
    {
        // server
        if (strcmp(curr, "servername") == 0)
            servMgr->serverName = cgi::unescape(arg);
        else if (strcmp(curr, "serveractive") == 0)
            servMgr->autoServe = getCGIargBOOL(arg);
        else if (strcmp(curr, "port") == 0)
            newPort = getCGIargINT(arg);
        else if (strcmp(curr, "icymeta") == 0)
        {
            int iv = getCGIargINT(arg);
            if (iv < 0) iv = 0;
            else if (iv > 16384) iv = 16384;

            chanMgr->icyMetaInterval = iv;
        }else if (strcmp(curr, "passnew") == 0)
            strcpy_s(servMgr->password, _countof(servMgr->password), arg);
        else if (strcmp(curr, "root") == 0)
            servMgr->isRoot = getCGIargBOOL(arg);
        else if (strcmp(curr, "brroot") == 0)
            brRoot = getCGIargBOOL(arg);
        else if (strcmp(curr, "getupd") == 0)
            getUpd = getCGIargBOOL(arg);
        else if (strcmp(curr, "huint") == 0)
            chanMgr->setUpdateInterval(getCGIargINT(arg));
        else if (strcmp(curr, "forceip") == 0)
            servMgr->forceIP = arg;
        else if (strcmp(curr, "htmlPath") == 0)
        {
            strcpy_s(servMgr->htmlPath, _countof(servMgr->htmlPath), "html/");
            strcat_s(servMgr->htmlPath, _countof(servMgr->htmlPath), arg);
        }else if (strcmp(curr, "djmsg") == 0)
        {
            String msg;
            msg.set(arg, String::T_ESC);
            msg.convertTo(String::T_UNICODE);
            chanMgr->setBroadcastMsg(msg);
        }
        else if (strcmp(curr, "pcmsg") == 0)
        {
            servMgr->rootMsg.set(arg, String::T_ESC);
            servMgr->rootMsg.convertTo(String::T_UNICODE);
        }else if (strcmp(curr, "minpgnu") == 0)
            servMgr->minGnuIncoming = atoi(arg);
        else if (strcmp(curr, "maxpgnu") == 0)
            servMgr->maxGnuIncoming = atoi(arg);

        // connections
        else if (strcmp(curr, "maxcin") == 0)
            servMgr->maxControl = getCGIargINT(arg);
        else if (strcmp(curr, "maxsin") == 0)
            servMgr->maxServIn = getCGIargINT(arg);

        else if (strcmp(curr, "maxup") == 0)
            servMgr->maxBitrateOut = getCGIargINT(arg);
        else if (strcmp(curr, "maxrelays") == 0)
            servMgr->setMaxRelays(getCGIargINT(arg));
        else if (strcmp(curr, "maxdirect") == 0)
            servMgr->maxDirect = getCGIargINT(arg);
        else if (strcmp(curr, "maxrelaypc") == 0)
            chanMgr->maxRelaysPerChannel = getCGIargINT(arg);
        else if (strncmp(curr, "filt_", 5) == 0)
        {
            char *fs = curr+5;

            if (strncmp(fs, "ip", 2) == 0)        // ip must be first
            {
                currFilter = &servMgr->filters[servMgr->numFilters];
                currFilter->init();
                currFilter->host.fromStrIP(arg, DEFAULT_PORT);
                if ((currFilter->host.ip) && (servMgr->numFilters < (ServMgr::MAX_FILTERS-1)))
                {
                    servMgr->numFilters++;
                    servMgr->filters[servMgr->numFilters].init();   // clear new entry
                }
            }else if (strncmp(fs, "bn", 2) == 0)
                currFilter->flags |= ServFilter::F_BAN;
            else if (strncmp(fs, "pr", 2) == 0)
                currFilter->flags |= ServFilter::F_PRIVATE;
            else if (strncmp(fs, "nw", 2) == 0)
                currFilter->flags |= ServFilter::F_NETWORK;
            else if (strncmp(fs, "di", 2) == 0)
                currFilter->flags |= ServFilter::F_DIRECT;
        }
        else if (strcmp(curr, "channel_feed_url") == 0)
        {
            if (strcmp(arg, "") != 0)
            {
                String str(arg, String::T_ESC);
                str.convertTo(String::T_ASCII);
                servMgr->channelDirectory.addFeed(str.cstr());
            }
        }
        else if (strncmp(curr, "channel_feed_public", strlen("channel_feed_public")) == 0)
        {
            int index = atoi(curr + strlen("channel_feed_public"));
            servMgr->channelDirectory.setFeedPublic(index, true);
        }

        // client
        else if (strcmp(curr, "clientactive") == 0)
            servMgr->autoConnect = getCGIargBOOL(arg);
        else if (strcmp(curr, "yp") == 0)
        {
            if (!PCP_FORCE_YP)
            {
                String str(arg, String::T_ESC);
                str.convertTo(String::T_ASCII);
                servMgr->rootHost = str;
            }
        }
        else if (strcmp(curr, "deadhitage") == 0)
            chanMgr->deadHitAge = getCGIargINT(arg);
        else if (strcmp(curr, "refresh") == 0)
            servMgr->refreshHTML = getCGIargINT(arg);
        else if (strcmp(curr, "public_directory") == 0)
            servMgr->publicDirectoryEnabled = true;
        else if (strcmp(curr, "auth") == 0)
        {
            if (strcmp(arg, "cookie") == 0)
                servMgr->authType = ServMgr::AUTH_COOKIE;
            else if (strcmp(arg, "http") == 0)
                servMgr->authType = ServMgr::AUTH_HTTPBASIC;
        }else if (strcmp(curr, "expire") == 0)
        {
            if (strcmp(arg, "session") == 0)
                servMgr->cookieList.neverExpire = false;
            else if (strcmp(arg, "never") == 0)
                servMgr->cookieList.neverExpire = true;
        }

        else if (strcmp(curr, "logDebug") == 0)
            showLog |= atoi(arg) ? (1<<LogBuffer::T_DEBUG) : 0;
        else if (strcmp(curr, "logErrors") == 0)
            showLog |= atoi(arg) ? (1<<LogBuffer::T_ERROR) : 0;
        else if (strcmp(curr, "logNetwork") == 0)
            showLog |= atoi(arg) ? (1<<LogBuffer::T_NETWORK) : 0;
        else if (strcmp(curr, "logChannel") == 0)
            showLog |= atoi(arg) ? (1<<LogBuffer::T_CHANNEL) : 0;

        else if (strcmp(curr, "allowHTML1") == 0)
            allowServer1 |= atoi(arg) ? (ALLOW_HTML) : 0;
        else if (strcmp(curr, "allowNetwork1") == 0)
            allowServer1 |= atoi(arg) ? (ALLOW_NETWORK) : 0;
        else if (strcmp(curr, "allowBroadcast1") == 0)
            allowServer1 |= atoi(arg) ? (ALLOW_BROADCAST) : 0;
        else if (strcmp(curr, "allowDirect1") == 0)
            allowServer1 |= atoi(arg) ? (ALLOW_DIRECT) : 0;

        else if (strcmp(curr, "allowHTML2") == 0)
            allowServer2 |= atoi(arg) ? (ALLOW_HTML) : 0;
        else if (strcmp(curr, "allowBroadcast2") == 0)
            allowServer2 |= atoi(arg) ? (ALLOW_BROADCAST) : 0;
    }

    servMgr->showLog = showLog;
    servMgr->allowServer1 = allowServer1;
    servMgr->allowServer2 = allowServer2;

    if (servMgr->serverHost.port != newPort)
    {
        std::string ipstr;
        if (http.headers.get("Host") != "")
        {
            auto vec = str::split(http.headers.get("Host"), ":");
            ipstr =  vec[0]+":"+std::to_string(newPort);
        }else
        {
            ipstr = Host(ClientSocket::getIP(NULL), newPort).str();
        }
        jumpStr.sprintf("http://%s/%s/settings.html", ipstr.c_str(), servMgr->htmlPath);

        servMgr->serverHost.port = newPort;
        servMgr->restartServer = true;

        sys->sleep(500); // give server time to restart itself
    }else
    {
        jumpStr.sprintf("/%s/settings.html", servMgr->htmlPath);
    }

    peercastInst->saveSettings();
    peercast::notifyMessage(ServMgr::NT_PEERCAST, "�ݒ��ۑ����܂����B");
    peercastApp->updateSettings();

    if ((servMgr->isRoot) && (brRoot))
        servMgr->broadcastRootSettings(getUpd);
}

void Servent::CMD_fetch(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    char arg[MAX_CGI_LEN];
    char curr[MAX_CGI_LEN];

    ChanInfo info;
    String curl;

    char *cp = cmd;
    while (cp=nextCGIarg(cp, curr, arg))
    {
        if (strcmp(curr, "url") == 0)
        {
            curl.set(arg, String::T_ESC);
            curl.convertTo(String::T_UNICODE);
        }else if (strcmp(curr, "name") == 0)
        {
            info.name.set(arg, String::T_ESC);
            info.name.convertTo(String::T_UNICODE);
        }else if (strcmp(curr, "desc") == 0)
        {
            info.desc.set(arg, String::T_ESC);
            info.desc.convertTo(String::T_UNICODE);
        }else if (strcmp(curr, "genre") == 0)
        {
            info.genre.set(arg, String::T_ESC);
            info.genre.convertTo(String::T_UNICODE);
        }else if (strcmp(curr, "contact") == 0)
        {
            info.url.set(arg, String::T_ESC);
            info.url.convertTo(String::T_UNICODE);
        }else if (strcmp(curr, "bitrate") == 0)
        {
            info.bitrate = atoi(arg);
        }else if (strcmp(curr, "type") == 0)
        {
            ChanInfo::TYPE type = ChanInfo::getTypeFromStr(arg);
            info.contentType = type;
            info.contentTypeStr = ChanInfo::getTypeStr(type);
            info.MIMEType = ChanInfo::getMIMEType(type);
            info.streamExt = ChanInfo::getTypeExt(type);
        }
    }

    info.bcID = chanMgr->broadcastID;
    // id ���Z�b�g����Ă��Ȃ��`�����l��������Ƃ��낢��܂����̂ŁA��
    // �O�ɐݒ肵�Ă���o�^����B
    info.id = chanMgr->broadcastID;
    info.id.encode(NULL, info.name, info.genre, info.bitrate);

    Channel *c = chanMgr->createChannel(info, NULL);
    if (c)
        c->startURL(curl.cstr());

    jumpStr.sprintf("/%s/channels.html", servMgr->htmlPath);
}

void Servent::CMD_stopserv(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    char arg[MAX_CGI_LEN];
    char curr[MAX_CGI_LEN];

    char *cp = cmd;
    while (cp=nextCGIarg(cp, curr, arg))
    {
        if (strcmp(curr, "index") == 0)
        {
            Servent *s = servMgr->findServentByIndex(atoi(arg));
            if (s)
                s->abort();
        }
    }
    jumpStr.sprintf("/%s/connections.html", servMgr->htmlPath);
}

void Servent::CMD_hitlist(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    bool stayConnected=hasCGIarg(cmd, "relay");

    int index = 0;
    ChanHitList *chl = chanMgr->hitlist;
    while (chl)
    {
        if (chl->isUsed())
        {
            char tmp[64];
            sprintf_s(tmp, _countof(tmp), "c%d=", index);
            if (cmpCGIarg(cmd, tmp, "1"))
            {
                Channel *c;
                if (!(c=chanMgr->findChannelByID(chl->info.id)))
                {
                    c = chanMgr->createChannel(chl->info, NULL);
                    if (!c)
                        throw StreamException("out of channels");
                    c->stayConnected = stayConnected;
                    c->startGet();
                }
            }
        }
        chl = chl->next;
        index++;
    }

    const char *findArg = getCGIarg(cmd, "keywords=");

    if (hasCGIarg(cmd, "relay"))
    {
        sys->sleep(500);
        jumpStr.sprintf("/%s/channels.html", servMgr->htmlPath);
    }
}

void Servent::CMD_clear(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    char arg[MAX_CGI_LEN];
    char curr[MAX_CGI_LEN];

    char *cp = cmd;
    while (cp=nextCGIarg(cp, curr, arg))
    {
        if (strcmp(curr, "hostcache") == 0)
            servMgr->clearHostCache(ServHost::T_SERVENT);
        else if (strcmp(curr, "hitlists") == 0)
            chanMgr->clearHitLists();
        else if (strcmp(curr, "packets") == 0)
            stats.clearRange(Stats::PACKETSSTART, Stats::PACKETSEND);
        else if (strcmp(curr, "channels") == 0)
            chanMgr->closeIdles();
    }

    if (!http.headers.get("Referer").empty())
        jumpStr.sprintf("%s", http.headers.get("Referer").c_str());
    else
        jumpStr.sprintf("/%s/index.html", servMgr->htmlPath);
}

void Servent::CMD_upgrade(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    if (servMgr->downloadURL[0])
    {
        jumpStr.sprintf("/admin?cmd=redirect&url=%s", servMgr->downloadURL);
    }
}

void Servent::CMD_connect(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    Servent *s = servMgr->servents;
    {
        char tmp[64];
        sprintf_s(tmp, _countof(tmp), "c%d=", s->serventIndex);
        if (cmpCGIarg(cmd, tmp, "1"))
        {
            if (hasCGIarg(cmd, "stop"))
                s->thread.active = false;
        }
        s=s->next;
    }
    jumpStr.sprintf("/%s/connections.html", servMgr->htmlPath);
}

void Servent::CMD_shutdown(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    http.send(HTTPResponse::ok({}, "Server is shutting down..."));
    servMgr->shutdownTimer = 1;
}

void Servent::CMD_stop(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    char arg[MAX_CGI_LEN];
    char curr[MAX_CGI_LEN];

    GnuID id;
    char *cp = cmd;
    while (cp=nextCGIarg(cp, curr, arg))
    {
        if (strcmp(curr, "id") == 0)
            id.fromStr(arg);
    }

    Channel *c = chanMgr->findChannelByID(id);
    if (c)
        c->thread.active = false;

    sys->sleep(500);
    jumpStr.sprintf("/%s/channels.html", servMgr->htmlPath);
}

void Servent::CMD_bump(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    cgi::Query query(cmd);
    GnuID id;
    std::string ip;

    if (!query.get("id").empty())
        id.fromStr(query.get("id").c_str());

    ip = query.get("ip");
    Host designation;
    designation.fromStrIP(ip.c_str(), 7144);

    Channel *c = chanMgr->findChannelByID(id);
    if (c)
    {
        if (!ip.empty())
        {
            ChanHitList* chl = chanMgr->findHitList(c->info);
            ChanHit theHit;

            if (chl)
            {
                chl->forEachHit(
                    [&] (ChanHit* hit)
                    {
                        if (hit->host == designation)
                            theHit = *hit;
                    });
            }

            if (theHit.host.ip != 0)
            {
                c->designatedHost = theHit;
            } else
            {
                // ChanHit���ł���������
                c->designatedHost.init();
                c->designatedHost.host = c->designatedHost.rhost[0] = designation;
            }
        }
        c->bump = true;
    }

    if (!http.headers.get("Referer").empty())
    {
        jumpStr.sprintf("%s", http.headers.get("Referer").c_str());
    }else
    {
        jumpStr.sprintf("/%s/channels.html", servMgr->htmlPath);
    }
}

void Servent::CMD_keep(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    char arg[MAX_CGI_LEN];
    char curr[MAX_CGI_LEN];

    GnuID id;
    char *cp = cmd;
    while (cp=nextCGIarg(cp, curr, arg))
    {
        if (strcmp(curr, "id") == 0)
            id.fromStr(arg);
    }

    Channel *c = chanMgr->findChannelByID(id);
    if (c)
        c->stayConnected = !c->stayConnected;

    jumpStr.sprintf("/%s/channels.html", servMgr->htmlPath);
}

void Servent::CMD_relay(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    char arg[MAX_CGI_LEN];
    char curr[MAX_CGI_LEN];

    ChanInfo info;
    char *cp = cmd;
    while (cp=nextCGIarg(cp, curr, arg))
    {
        if (strcmp(curr, "id") == 0)
            info.id.fromStr(arg);
    }

    if (!chanMgr->findChannelByID(info.id))
    {
        ChanHitList *chl = chanMgr->findHitList(info);
        if (!chl)
            throw StreamException("channel not found");

        Channel *c = chanMgr->createChannel(chl->info, NULL);
        if (!c)
            throw StreamException("out of channels");

        c->stayConnected = true;
        c->startGet();
    }

    jumpStr.sprintf("/%s/channels.html", servMgr->htmlPath);
}

void Servent::CMD_net_add(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    char arg[MAX_CGI_LEN];
    char curr[MAX_CGI_LEN];

    GnuID id;
    while (cmd=nextCGIarg(cmd, curr, arg))
    {
        if (strcmp(curr, "ip") == 0)
        {
            Host h;
            h.fromStrIP(arg, DEFAULT_PORT);
            if (servMgr->addOutgoing(h, id, true))
                LOG_NETWORK("Added connection: %s", arg);
        }else if (strcmp(curr, "id") == 0)
        {
            id.fromStr(arg);
        }
    }
}

void Servent::CMD_logout(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    jumpStr.sprintf("%s", "/");
    servMgr->cookieList.remove(cookie);
}

void Servent::CMD_login(char *cmd, HTTP& http, HTML& html, String& jumpStr)
{
    GnuID id;
    char idstr[64];
    id.generate();
    id.toStr(idstr);

    cookie.set(idstr, sock->host.ip);
    servMgr->cookieList.add(cookie);

    http.writeLine(HTTP_SC_FOUND);
    if (servMgr->cookieList.neverExpire)
        http.writeLineF("%s id=%s; path=/; expires=\"Mon, 01-Jan-3000 00:00:00 GMT\";", HTTP_HS_SETCOOKIE, idstr);
    else
        http.writeLineF("%s id=%s; path=/;", HTTP_HS_SETCOOKIE, idstr);
    if (!http.headers.get("Referer").empty())
        http.writeLineF("Location: %s", http.headers.get("Referer").c_str());
    else
        http.writeLineF("Location: /%s/index.html", servMgr->htmlPath);
    http.writeLine("");
}

void Servent::handshakeCMD(char *cmd)
{
    String jumpStr;

    HTTP http(*sock);
    HTML html("", *sock);

    if (!handshakeAuth(http, cmd, true))
        return;

    try
    {
        if (cmpCGIarg(cmd, "cmd=", "redirect"))
        {
            CMD_redirect(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "viewxml"))
        {
            CMD_viewxml(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "clearlog"))
        {
            CMD_clearlog(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "save"))
        {
            CMD_save(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "reg"))
        {
            CMD_reg(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "edit_bcid"))
        {
            CMD_edit_bcid(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "add_bcid"))
        {
            CMD_add_bcid(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "apply"))
        {
            CMD_apply(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "fetch"))
        {
            CMD_fetch(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "stopserv"))
        {
            CMD_stopserv(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "hitlist"))
        {
            CMD_hitlist(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "clear"))
        {
            CMD_clear(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "upgrade"))
        {
            CMD_upgrade(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "connect"))
        {
            CMD_connect(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "shutdown"))
        {
            CMD_shutdown(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "stop"))
        {
            CMD_stop(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "bump"))
        {
            CMD_bump(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "keep"))
        {
            CMD_keep(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "relay"))
        {
            CMD_relay(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "net=", "add"))
        {
            CMD_net_add(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "logout"))
        {
            CMD_logout(cmd, http, html, jumpStr);
        }else if (cmpCGIarg(cmd, "cmd=", "login"))
        {
            CMD_login(cmd, http, html, jumpStr);
        }else{
            jumpStr.sprintf("/%s/index.html", servMgr->htmlPath);
        }
    }catch (HTTPException &e)
    {
        http.writeLine(e.msg);
        http.writeLineF("%s %s", HTTP_HS_SERVER, PCX_AGENT);
        http.writeLineF("%s %s", HTTP_HS_CONTENT, "text/html");
        http.writeLine("");

        html.startTagEnd("h1", "ERROR - %s", e.msg);

        LOG_ERROR("html: %s", e.msg);
    }

    if (jumpStr != "")
    {
        String jmp(jumpStr, String::T_HTML);
        jmp.convertTo(String::T_ASCII);
        html.locateTo(jmp.cstr());
    }
}

// -----------------------------------
static XML::Node *createChannelXML(Channel *c)
{
    XML::Node *n = c->info.createChannelXML();
    n->add(c->createRelayXML(true));
    n->add(c->info.createTrackXML());
//  n->add(c->info.createServentXML());
    return n;
}

// -----------------------------------
static XML::Node *createChannelXML(ChanHitList *chl)
{
    XML::Node *n = chl->info.createChannelXML();
    n->add(chl->createXML());
    n->add(chl->info.createTrackXML());
//  n->add(chl->info.createServentXML());
    return n;
}

// -----------------------------------
void Servent::handshakeXML()
{
    XML xml;

    XML::Node *rn = new XML::Node("peercast");
    xml.setRoot(rn);

    rn->add(new XML::Node("servent uptime=\"%d\"", servMgr->getUptime()));

    rn->add(new XML::Node("bandwidth out=\"%d\" in=\"%d\"",
        stats.getPerSecond(Stats::BYTESOUT)-stats.getPerSecond(Stats::LOCALBYTESOUT),
        stats.getPerSecond(Stats::BYTESIN)-stats.getPerSecond(Stats::LOCALBYTESIN)
        ));

    rn->add(new XML::Node("connections total=\"%d\" relays=\"%d\" direct=\"%d\"", servMgr->totalConnected(), servMgr->numStreams(Servent::T_RELAY, true), servMgr->numStreams(Servent::T_DIRECT, true)));

    XML::Node *an = new XML::Node("channels_relayed total=\"%d\"", chanMgr->numChannels());
    rn->add(an);

    Channel *c = chanMgr->channel;
    while (c)
    {
        if (c->isActive())
            an->add(createChannelXML(c));
        c=c->next;
    }

    // add public channels
    {
        XML::Node *fn = new XML::Node("channels_found total=\"%d\"", chanMgr->numHitLists());
        rn->add(fn);

        ChanHitList *chl = chanMgr->hitlist;
        while (chl)
        {
            if (chl->isUsed())
                fn->add(createChannelXML(chl));
            chl = chl->next;
        }
    }

#if 0
    if (servMgr->isRoot)
    {
        // add private channels
        {
            XML::Node *pn = new XML::Node("priv_channels");
            rn->add(pn);

            ChanHitList *chl = chanMgr->hitlist;
            while (chl)
            {
                if (chl->isUsed())
                    if (chl->info.isPrivate())
                        pn->add(createChannelXML(chl));
                chl = chl->next;
            }
        }
    }
#endif

    XML::Node *hc = new XML::Node("host_cache");
    for (int i=0; i<ServMgr::MAX_HOSTCACHE; i++)
    {
        ServHost *sh = &servMgr->hostCache[i];
        if (sh->type != ServHost::T_NONE)
        {
            char ipstr[64];
            sh->host.toStr(ipstr);

            hc->add(new XML::Node("host ip=\"%s\" type=\"%s\" time=\"%d\"", ipstr, ServHost::getTypeStr(sh->type), sh->time));
        }
    }
    rn->add(hc);

    // calculate content-length
    DummyStream ds;
    xml.write(ds);

    // set line-feed code to CRLF (for HTTP header)
    bool bWriteCRLF = sock->writeCRLF;
    sock->writeCRLF = true;

    // write HTTP response header
    sock->writeLine(HTTP_SC_OK);
    sock->writeLineF("%s %s", HTTP_HS_SERVER, PCX_AGENT);
    sock->writeLineF("%s %s", HTTP_HS_CONTENT, MIME_XML);
    sock->writeLineF("%s %d", HTTP_HS_LENGTH, ds.getLength());
    sock->writeLine("Connection: close");
    sock->writeLine("");

    // revert setting
    sock->writeCRLF = bWriteCRLF;

    // write HTTP body
    xml.write(*sock);
}

// -----------------------------------
void Servent::readICYHeader(HTTP &http, ChanInfo &info, char *pwd, size_t plen)
{
    char *arg = http.getArgStr();
    if (!arg) return;

    if (http.isHeader("x-audiocast-name") || http.isHeader("icy-name") || http.isHeader("ice-name"))
    {
        info.name.set(arg, String::T_ASCII);
        info.name.convertTo(String::T_UNICODE);
    }else if (http.isHeader("x-audiocast-url") || http.isHeader("icy-url") || http.isHeader("ice-url"))
        info.url.set(arg, String::T_ASCII);
    else if (http.isHeader("x-audiocast-bitrate") || (http.isHeader("icy-br")) || http.isHeader("ice-bitrate") || http.isHeader("icy-bitrate"))
        info.bitrate = atoi(arg);
    else if (http.isHeader("x-audiocast-genre") || http.isHeader("ice-genre") || http.isHeader("icy-genre"))
    {
        info.genre.set(arg, String::T_ASCII);
        info.genre.convertTo(String::T_UNICODE);
    }else if (http.isHeader("x-audiocast-description") || http.isHeader("ice-description"))
    {
        info.desc.set(arg, String::T_ASCII);
        info.desc.convertTo(String::T_UNICODE);

    }else if (http.isHeader("Authorization")) {
        if (pwd)
            http.getAuthUserPass(NULL, pwd, 0, plen);
    }
    else if (http.isHeader(PCX_HS_CHANNELID))
        info.id.fromStr(arg);
    else if (http.isHeader("ice-password"))
    {
        if (pwd)
            if (strlen(arg) < 64)
                strcpy_s(pwd, 64, arg);
    }else if (http.isHeader("content-type"))
    {
        if (stristr(arg, MIME_OGG))
            info.contentType = ChanInfo::T_OGG;
        else if (stristr(arg, MIME_XOGG))
            info.contentType = ChanInfo::T_OGG;

        else if (stristr(arg, MIME_MP3))
            info.contentType = ChanInfo::T_MP3;
        else if (stristr(arg, MIME_XMP3))
            info.contentType = ChanInfo::T_MP3;

        else if (stristr(arg, MIME_WMA))
            info.contentType = ChanInfo::T_WMA;
        else if (stristr(arg, MIME_WMV))
            info.contentType = ChanInfo::T_WMV;
        else if (stristr(arg, MIME_ASX))
            info.contentType = ChanInfo::T_ASX;

        else if (stristr(arg, MIME_NSV))
            info.contentType = ChanInfo::T_NSV;
        else if (stristr(arg, MIME_RAW))
            info.contentType = ChanInfo::T_RAW;

        else if (stristr(arg, MIME_MMS))
            info.srcProtocol = ChanInfo::SP_MMS;
        else if (stristr(arg, MIME_XPCP))
            info.srcProtocol = ChanInfo::SP_PCP;
        else if (stristr(arg, MIME_XPEERCAST))
            info.srcProtocol = ChanInfo::SP_PEERCAST;

        else if (stristr(arg, MIME_XSCPLS))
            info.contentType = ChanInfo::T_PLS;
        else if (stristr(arg, MIME_PLS))
            info.contentType = ChanInfo::T_PLS;
        else if (stristr(arg, MIME_XPLS))
            info.contentType = ChanInfo::T_PLS;
        else if (stristr(arg, MIME_M3U))
            info.contentType = ChanInfo::T_PLS;
        else if (stristr(arg, MIME_MPEGURL))
            info.contentType = ChanInfo::T_PLS;
        else if (stristr(arg, MIME_TEXT))
            info.contentType = ChanInfo::T_PLS;
    }
}

// -----------------------------------
// Windows Media HTTP Push Distribution Protocol
#define ASSERT(x)
void Servent::handshakeWMHTTPPush(HTTP& http, const std::string& path)
{
    // At this point, http has read all the headers.

    ASSERT(http.headers["CONTENT-TYPE"] == "application/x-wms-pushsetup");
    LOG_DEBUG("%s", nlohmann::json(http.headers.m_headers).dump().c_str());

    int size = std::atoi(http.headers.get("Content-Length").c_str());

    // �G���R�[�_�[�̐ݒ�v����ǂށB0 �o�C�g�̋�̐ݒ�v�������@�B
    unique_ptr<char> buffer(new char[size + 1]);
    http.read(buffer.get(), size);
    buffer.get()[size] = '\0';
    LOG_DEBUG("setup: %s", str::inspect(buffer.get()).c_str());

    // �킩�����ӂ������
    http.writeLine("HTTP/1.1 204 No Content"); // ������Ă����� CRLF �o��H
    std::vector< std::pair<const char*,const char*> > headers =
        {
            { "Server"         , "Cougar/9.01.01.3814" },
            { "Cache-Control"  , "no-cache" },
            { "Supported"      , "com.microsoft.wm.srvppair, "
              "com.microsoft.wm.sswitch, "
              "com.microsoft.wm.predstrm, "
              "com.microsoft.wm.fastcache, "
              "com.microsoft.wm.startupprofile" },
            { "Content-Length" , "0" },
            { "Connection"     , "Keep-Alive" },
        };
    for (auto hline : headers)
    {
        http.writeLineF("%s: %s", hline.first, hline.second);
    }
    http.writeLine("");

    // -----------------------------------------
    http.reset();
    http.readRequest();
    LOG_DEBUG("Request line: %s", http.cmdLine);

    http.readHeaders();
    LOG_DEBUG("Setup: %s", nlohmann::json(http.headers.m_headers).dump().c_str());

    ASSERT(http.headers["CONTENT-TYPE"] == "application/x-wms-pushstart");

    // -----------------------------------------

    // User-Agent �w�b�_�[������� agent ���Z�b�g
    if (http.headers.get("User-Agent") != "")
        this->agent = http.headers.get("User-Agent");

    auto vec = str::split(cgi::unescape(path.substr(1)), ";");
    if (vec.size() == 0)
        throw HTTPException(HTTP_SC_BADREQUEST, 400);

    ChanInfo info;
    info.setContentType(ChanInfo::getTypeFromStr("WMV"));
    info.name = vec[0];
    if (vec.size() > 1) info.genre = vec[1];
    if (vec.size() > 2) info.desc  = vec[2];
    if (vec.size() > 3) info.url   = vec[3];

    info.id = chanMgr->broadcastID;
    info.id.encode(NULL, info.name.cstr(), info.genre.cstr(), info.bitrate);

    Channel *c = chanMgr->findChannelByID(info.id);
    if (c)
    {
        LOG_CHANNEL("WMHTTP Push channel already active, closing old one");
        c->thread.shutdown();
    }

    info.comment = chanMgr->broadcastMsg;
    info.bcID    = chanMgr->broadcastID;

    c = chanMgr->createChannel(info, NULL);
    if (!c)
        throw HTTPException(HTTP_SC_SERVERERROR, 500);

    c->startWMHTTPPush(sock);
    sock = NULL;    // socket is taken over by channel, so don`t close it
}

// -----------------------------------
ChanInfo Servent::createChannelInfo(GnuID broadcastID, const String& broadcastMsg, cgi::Query& query, const std::string& contentType)
{
    ChanInfo info;

    auto type = ChanInfo::getTypeFromStr(query.get("type").c_str());
    // type ����A���邢�͔F���ł��Ȃ��ꍇ�̓��N�G�X�g�� Content-Type ���玩�����ʂ���
    if (type == ChanInfo::T_UNKNOWN)
        type = ChanInfo::getTypeFromMIME(contentType);

    info.setContentType(type);
    info.name    = query.get("name");
    info.genre   = query.get("genre");
    info.desc    = query.get("desc");
    info.url     = query.get("url");
    info.bitrate = atoi(query.get("bitrate").c_str());
    info.comment = query.get("comment").empty() ? broadcastMsg : query.get("comment");

    info.id = broadcastID;
    info.id.encode(NULL, info.name.cstr(), info.genre.cstr(), info.bitrate);
    info.bcID = broadcastID;

    return info;
}

// -----------------------------------
// HTTP Push ����
void Servent::handshakeHTTPPush(const std::string& args)
{
    using namespace matroska;
    using namespace cgi;

    Query query(args);

    // HTTP �w�b�_�[��S�ēǂݍ���
    HTTP http(*sock);
    http.readHeaders();

    if (query.get("name").empty())
    {
        LOG_ERROR("handshakeHTTPPush: name parameter is mandatory");
        throw HTTPException(HTTP_SC_BADREQUEST, 400);
    }

    ChanInfo info = createChannelInfo(chanMgr->broadcastID, chanMgr->broadcastMsg, query, http.headers.get("Content-Type"));

    Channel *c = chanMgr->findChannelByID(info.id);
    if (c)
    {
        LOG_CHANNEL("HTTP Push channel already active, closing old one");
        c->thread.shutdown();
    }
    // �����ŃV���b�g�_�E���҂��Ȃ��Ă����́H

    c = chanMgr->createChannel(info, NULL);
    if (!c)
        throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

    bool chunked = (http.headers.get("Transfer-Encoding") == "chunked");
    c->startHTTPPush(sock, chunked);
    sock = NULL;    // socket is taken over by channel, so don`t close it
}

// -----------------------------------
void Servent::handshakeICY(Channel::SRC_TYPE type, bool isHTTP)
{
    ChanInfo info;

    HTTP http(*sock);

    // default to mp3 for shoutcast DSP (doesn`t send content-type)
    if (type == Channel::SRC_SHOUTCAST)
        info.contentType = ChanInfo::T_MP3;

    while (http.nextHeader())
    {
        LOG_DEBUG("ICY %s", http.cmdLine);
        readICYHeader(http, info, loginPassword.cstr(), String::MAX_LEN);
    }

    // check password before anything else, if needed
    if (loginPassword != servMgr->password)
    {
        if (!sock->host.isLocalhost() || !loginPassword.isEmpty())
            throw HTTPException(HTTP_SC_UNAUTHORIZED, 401);
    }

    // we need a valid IP address before we start
    servMgr->checkFirewall();

    // attach channel ID to name, channel ID is also encoded with IP address
    // to help prevent channel hijacking.

    info.id = chanMgr->broadcastID;
    info.id.encode(NULL, info.name.cstr(), loginMount.cstr(), info.bitrate);

    LOG_DEBUG("Incoming source: %s : %s", info.name.cstr(), info.getTypeStr());
    if (isHTTP)
        sock->writeStringF("%s\n\n", HTTP_SC_OK);
    else
        sock->writeLine("OK");

    Channel *c = chanMgr->findChannelByID(info.id);
    if (c)
    {
        LOG_CHANNEL("ICY channel already active, closing old one");
        c->thread.shutdown();
    }

    info.comment = chanMgr->broadcastMsg;
    info.bcID = chanMgr->broadcastID;

    c = chanMgr->createChannel(info, loginMount.cstr());
    if (!c)
        throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

    c->startICY(sock, type);
}

// -----------------------------------
void Servent::handshakeLocalFile(const char *fn)
{
    String fileName;

    fileName = peercastApp->getPath();
    fileName.append(fn);

    LOG_DEBUG("Writing HTML file: %s", fileName.cstr());

    WriteBufferedStream bufferedSock(sock);
    HTML html("", bufferedSock);

    if (fileName.contains(".htm"))
    {
        if (str::contains(fn, "play.html"))
        {
            auto vec = str::split(fn, "?");
            if (vec.size() != 2)
                throw HTTPException(HTTP_SC_BADREQUEST, 400);

            String id = cgi::Query(vec[1]).get("id").c_str();

            if (id.isEmpty())
                throw HTTPException(HTTP_SC_BADREQUEST, 400);

            ChanInfo info;
            if (!servMgr->getChannel(id.cstr(), info, true))
                throw HTTPException(HTTP_SC_NOTFOUND, 404);
        }

        char *args = strstr(fileName.cstr(), "?");
        if (args)
            *args++ = 0;
        html.writeOK(MIME_HTML);
        html.writeTemplate(fileName.cstr(), args);
    }else if (fileName.contains(".css"))
    {
        html.writeRawFile(fileName.cstr(), MIME_CSS);
    }else if (fileName.contains(".jpg"))
    {
        html.writeRawFile(fileName.cstr(), MIME_JPEG);
    }else if (fileName.contains(".gif"))
    {
        html.writeRawFile(fileName.cstr(), MIME_GIF);
    }else if (fileName.contains(".png"))
    {
        html.writeRawFile(fileName.cstr(), MIME_PNG);
    }else if (fileName.contains(".js"))
    {
        html.writeRawFile(fileName.cstr(), MIME_JS);
    }else
    {
        throw HTTPException(HTTP_SC_NOTFOUND, 404);
    }
}

// -----------------------------------
void Servent::handshakeRemoteFile(const char *dirName)
{
    ClientSocket *rsock = sys->createSocket();
    if (!rsock)
        throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

    const char *hostName = "www.peercast.org";  // hardwired for "security"

    Host host;
    host.fromStrName(hostName, 80);

    if (host.ip == 0)
    {
        LOG_ERROR("handshakeRemoteFile: lookup failed for %s", hostName);
        throw HTTPException(HTTP_SC_BADGATEWAY, 502);
    }

    rsock->open(host);
    rsock->connect();

    HTTP rhttp(*rsock);

    rhttp.writeLineF("GET /%s HTTP/1.0", dirName);
    rhttp.writeLineF("%s %s", HTTP_HS_HOST, hostName);
    rhttp.writeLineF("%s %s", HTTP_HS_CONNECTION, "close");
    rhttp.writeLineF("%s %s", HTTP_HS_ACCEPT, "*/*");
    rhttp.writeLine("");

    String contentType;
    bool isTemplate = false;
    while (rhttp.nextHeader())
    {
        char *arg = rhttp.getArgStr();
        if (arg)
        {
            if (rhttp.isHeader("content-type"))
                contentType = arg;
        }
    }

    MemoryStream mem(100*1024);
    while (!rsock->eof())
    {
        int len=0;
        char buf[4096];
        len = rsock->readUpto(buf, sizeof(buf));
        if (len == 0)
            break;
        else
            mem.write(buf, len);
    }
    rsock->close();

    int fileLen = mem.getPosition();
    mem.len = fileLen;
    mem.rewind();

    if (contentType.contains(MIME_HTML))
        isTemplate = true;

    sock->writeLine(HTTP_SC_OK);
    sock->writeLineF("%s %s", HTTP_HS_SERVER, PCX_AGENT);
    sock->writeLineF("%s %s", HTTP_HS_CACHE, "no-cache");
    sock->writeLineF("%s %s", HTTP_HS_CONNECTION, "close");
    sock->writeLineF("%s %s", HTTP_HS_CONTENT, contentType.cstr());

    sock->writeLine("");

    if (isTemplate)
    {
        Template().readTemplate(mem, sock, 0);
    }else
        sock->write(mem.buf, fileLen);
}

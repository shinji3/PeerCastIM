#ifndef MOCKPEERCAST_H
#define MOCKPEERCAST_H

#include "peercast.h"
#include "mocksys.h"

class MockPeercastInstance : public PeercastInstance
{
public:
    Sys* createSys() override
    {
        return new MockSys();
    }
};

class MockPeercastApplication : public PeercastApplication
{
public:
    const char* APICALL getIniFilename() override
    {
        return "";
    }

    const char* APICALL getPath() override
    {
        return "";
    }

    const char* APICALL getClientTypeOS() override
    {
        return PCX_OS_WIN32;
    }

    void APICALL printLog(LogBuffer::TYPE t, const char* str) override
    {
    }
};

#endif

#ifndef MOCKSYS_H
#define MOCKSYS_H

#include "sys.h"

class MockSys : public Sys
{
public:
    MockSys()
    {
    }

    class ClientSocket* createSocket() override
    {
        return NULL;
    }

    bool startThread(class ThreadInfo*) override
    {
        return false;
    }

    void sleep(int) override
    {
    }

    void appMsg(long, long = 0) override
    {
    }

    unsigned int getTime() override
    {
        return 0;
    }

    double getDTime() override
    {
        return 0;
    }

    unsigned int rnd() override
    {
        return 123456789;
    }

    // URL���u���E�U�⃁�[���ŊJ���B
    void getURL(const char*) override
    {
    }

    void exit() override
    {
    }

    bool hasGUI() override
    {
        return false;
    }

    // ���[�J���T�[�o�[��URL���J���B
    void callLocalURL(const char*, int) override
    {
    }

    // �t�@�C�����J���B
    void executeFile(const char*) override
    {
    }

    void endThread(ThreadInfo*) override
    {
    }

    void waitThread(ThreadInfo*, int timeout = 30000) override
    {
    }
};

#endif

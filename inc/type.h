#pragma once

#include <string>
#include <3ds.h>

#define THRD_STACK_SIZE 0x10000
#define SOCU_ALIGN 0x1000
#define SOCU_BUFFERSIZE 0x80000
static_assert(SOCU_BUFFERSIZE % SOCU_ALIGN == 0);

typedef void (*funcPtr)(void *);

class threadStatus
{
    public:
        threadStatus(){ svcCreateMutex(&statusLock, false); }
        ~threadStatus(){ svcCloseHandle(statusLock); }

    void setStatus(const std::string& newStatus)
    {
        svcWaitSynchronization(statusLock, U64_MAX);
        status = newStatus;
        svcReleaseMutex(statusLock);
    }

    void getStatus(std::string& out)
    {
        svcWaitSynchronization(statusLock, U64_MAX);
        out = status;
        svcReleaseMutex(statusLock);
    }

    private:
        Handle statusLock;
        std::string status;
};

class threadInfo
{
    public:
        threadInfo() { svcCreateMutex(&threadLock, false); }
        ~threadInfo() { svcCloseHandle(threadLock); }

        void lock() { svcWaitSynchronization(threadLock, U64_MAX); }
        void unlock() { svcReleaseMutex(threadLock); }

        bool running = false, finished = false;
        Thread thrd;
        ThreadFunc thrdFunc;
        void *argPtr = NULL;
        funcPtr drawFunc = NULL;
        threadStatus *status;
        int thrdID = 0;
        size_t stackSize = THRD_STACK_SIZE;

    private:
        Handle threadLock;
};

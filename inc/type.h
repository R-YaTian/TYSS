/*
 *  This file is part of TYSS.
 *  Copyright (C) 2024-2025 R-YaTian
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#pragma once

#include <string>
#include <3ds.h>

#define THRD_STACK_SIZE 0x10000
#define ZIP_THRD_STACK_SIZE 0x100000
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

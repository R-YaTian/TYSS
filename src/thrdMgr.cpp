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

#include <3ds.h>
#include <vector>

#include "ui.h"
#include "type.h"
#include "gfx.h"
#include "fs.h"
#include "util.h"
#include "sys.h"

#include "thrdMgr.h"

/*Adapted the best I can from switch*/
ui::threadProcMngr::threadProcMngr()
{
    svcCreateMutex(&threadLock, false);
}

ui::threadProcMngr::~threadProcMngr()
{
    svcCloseHandle(threadLock);
    for(threadInfo *t : threads)
    {
        threadJoin(t->thrd, U64_MAX);
        threadFree(t->thrd);
        delete t->status;
        delete t;
    }
}

threadInfo *ui::threadProcMngr::newThread(ThreadFunc func, void *args, funcPtr _drawFunc, size_t stackSize)
{
    threadInfo *t = new threadInfo;
    t->status = new threadStatus;
    t->running = false;
    t->finished = false;
    t->thrdFunc = func;
    t->drawFunc = _drawFunc;
    t->argPtr = args;
    t->thrdID = cnt++;
    t->stackSize = stackSize;

    svcWaitSynchronization(threadLock, U64_MAX);
    threads.push_back(t);
    svcReleaseMutex(threadLock);

    return threads[threads.size() - 1];
}

void ui::threadProcMngr::update()
{
    if(!threads.empty())
    {
        threadInfo *t = threads[0];
        if(!t->running)
        {
            t->thrd = threadCreate(t->thrdFunc, t, t->stackSize, sys::threadPrio, sys::threadCore, false);
            t->running = true;
        }
        else if(t->finished)
        {
            threadJoin(t->thrd, U64_MAX);
            threadFree(t->thrd);
            delete t->status;
            delete t;
            svcWaitSynchronization(threadLock, U64_MAX);
            threads.erase(threads.begin());
            svcReleaseMutex(threadLock);
        }
    }
}

void ui::threadProcMngr::drawTop()
{
    C2D_DrawRectSolid(0, 0, GFX_DEPTH_DEFAULT, 400, 240, 0xBB000000);
}

void ui::threadProcMngr::drawBot()
{
    C2D_DrawRectSolid(0, 0, GFX_DEPTH_DEFAULT, 320, 240, 0xBB000000);

    if(++frameCount % 4 == 0 && ++lgFrame > 7)
        lgFrame = 0;

    if(clrAdd && (clrShft += 6) >= 0x72)
        clrAdd = false;
    else if(!clrAdd && (clrShft -= 3) <= 0x00)
        clrAdd = true;

    uint32_t glyphCol = 0xFF << 24 | (uint8_t)(0xC5 + (clrShft / 2)) << 16 | (uint8_t)(0x88 + clrShft) << 8 | 0x00;
    gfx::drawText(ui::loadGlyphArray[lgFrame], 4, 222, GFX_DEPTH_DEFAULT, 0.65f, glyphCol);
    
    threadInfo *t = threads[0];
    t->lock();
    if(t->running)
    {
        if(t->drawFunc)
            (*t->drawFunc)(t);
        else
        {
            std::string thrdstatus;
            t->status->getStatus(thrdstatus);
            int textWidth = gfx::getTextWidth(thrdstatus);
            int txtX = textWidth > 256 ? 32 : 160 - (gfx::getTextWidth(thrdstatus) / 2);

            gfx::drawTextWrap(thrdstatus, txtX, 114, GFX_DEPTH_DEFAULT, 0.5f, textWidth > 256 ? 256 : textWidth, 0xFFFFFFFF);
        }
    }
    t->unlock();
}

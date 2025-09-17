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
#include <cstdio>

#include "sys.h"

bool run = true;
int8_t sys::threadCore = -2;
bool sys::isNew3DS = false;
bool sys::isInstalled = false;
int sys::threadPrio = 0;

void sys::init()
{
    cfguInit();
    hidInit();
    amInit();
    pxiDevInit();
    aptInit();
    romfsInit();

    svcGetThreadPriority((s32 *)&sys::threadPrio, CUR_THREAD_HANDLE);
    --sys::threadPrio;

    uint8_t model = 0;
    CFGU_GetSystemModel(&model);
    if(model == 2 || model == 4 || model == 5)
    {
        sys::threadCore = 2;
        sys::isNew3DS = true;
    }

    char tmp[16];
    Result res = AM_GetTitleProductCode(MEDIATYPE_SD, 0x000400000B549300LL, tmp);
    if (R_SUCCEEDED(res))
        sys::isInstalled = true;
}

void sys::exit()
{
    cfguExit();
    hidExit();
    amExit();
    pxiDevExit();
    aptExit();
    romfsExit();
}

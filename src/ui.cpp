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
#include <citro2d.h>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <cstring>
#include <fstream>
#include <vector>

#include <stdio.h>

#include "data.h"
#include "gfx.h"
#include "ui.h"
#include "util.h"
#include "fs.h"
#include "sys.h"
#include "smdh.h"
#include "cfg.h"

#include "ui/ttlview.h"
#include "ui/ttl.h"

//8
const std::string ui::loadGlyphArray[] =
{
    "\ue020", "\ue021", "\ue022", "\ue023",
    "\ue024", "\ue025", "\ue026", "\ue027"
};

int ui::state = DAT, ui::prev = DAT;
ui::progressBar *ui::prog;
static ui::threadProcMngr *thrdMgr;
static ui::button *ok, *yes, *no;

const std::string TITLE_TEXT = "TY Save Studio - v1.0.5 ";

uint32_t ui::down = 0, ui::held = 0;
touchPosition ui::pos;

void ui::init()
{
    thrdMgr = new ui::threadProcMngr;
    ok = new ui::button("好 \ue000", 96, 184, 128, 32);
    yes = new ui::button("是 \ue000", 32, 184, 120, 32);
    no  = new ui::button("否 \ue001", 168, 184, 120, 32);
    prog = new ui::progressBar(100);

    gfx::setColor(std::get<bool>(cfg::config["lightback"]));
}

void ui::exit()
{
    ui::ttlExit();
    ui::extExit();
    ui::sysExit();
    ui::bossViewExit();
    ui::shrdExit();
    ui::setExit();
    delete thrdMgr;
    delete ok;
    delete yes;
    delete no;
}

void ui::drawUIBar(const std::string& txt, int screen, bool center)
{
    unsigned topX = 4, botX = 4;
    switch(screen)
    {
        case ui::SCREEN_TOP://top
            topX = 200 - (gfx::getTextWidth(txt) / 2);
            C2D_DrawRectSolid(0, 0, GFX_DEPTH_DEFAULT, 400, 16, gfx::rectLt);
            C2D_DrawRectSolid(0, 16, GFX_DEPTH_DEFAULT, 400, 1, gfx::divClr);
            gfx::drawText(txt, topX, 0, GFX_DEPTH_DEFAULT, 0.5f, gfx::txtCont);
            break;

        case ui::SCREEN_BOT://bottom
            botX = 160 - (gfx::getTextWidth(txt) / 2);
            C2D_DrawRectSolid(0, 224, GFX_DEPTH_DEFAULT, 320, 16, gfx::rectLt);
            C2D_DrawRectSolid(0, 223, GFX_DEPTH_DEFAULT, 320, 1, gfx::divClr);
            gfx::drawText(txt, botX, 224, GFX_DEPTH_DEFAULT, 0.5f, gfx::txtCont);
            break;
    }
}

void drawUI()
{
    gfx::frameBegin();
    gfx::frameStartTop();
    switch(ui::state)
    {
        case DAT:
            data::datDrawTop();
            break;

        case USR:
            ui::ttlDrawTop();
            break;

        case EXT:
            ui::extDrawTop();
            break;

        case SYS:
            ui::sysDrawTop();
            break;

        case BOS:
            ui::bossViewDrawTop();
            break;

        case SHR:
            ui::shrdDrawTop();
            break;

        case SET:
            ui::setDrawTop();
            break;

        default:
            break;
    }
    if(!thrdMgr->empty())
        thrdMgr->drawTop();

    gfx::frameStartBot();
    switch(ui::state)
    {
        case DAT:
            data::datDrawBot();
            break;

        case USR:
            ui::ttlDrawBot();
            break;

        case EXT:
            ui::extDrawBot();
            break;

        case SYS:
            ui::sysDrawBot();
            break;

        case BOS:
            ui::bossViewDrawBot();
            break;

        case SHR:
            ui::shrdDrawBot();
            break;

        case SET:
            ui::setDrawBottom();
            break;

        default:
            break;
    }
    if(!thrdMgr->empty())
        thrdMgr->drawBot();

    gfx::frameEnd();
}

bool ui::runApp()
{
    ui::updateInput();

    thrdMgr->update();
    if(thrdMgr->empty())
    {
        if(ui::padKeysDown() & KEY_START)
            return false;

        data::cartCheck();

        switch(state)
        {
            case USR:
                ui::ttlUpdate();
                break;

            case EXT:
                ui::extUpdate();
                break;

            case SYS:
                ui::sysUpdate();
                break;

            case BOS:
                ui::bossViewUpdate();
                break;

            case SHR:
                ui::shrdUpdate();
                break;

            case SET:
                ui::setUpdate();
                break;
        }
    }

    drawUI();

    return true;
}

void ui::showMessage(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    size_t size = std::vsnprintf(nullptr, 0, fmt, args) + 1;
    std::vector<char> buf(size);
    std::vsnprintf(buf.data(), size, fmt, args);

    va_end(args);

    std::string txt(buf.data(), buf.size() - 1);
    ui::message(txt, NULL, NULL);
}

void ui::newThread(ThreadFunc _thrdFunc, void *_args, funcPtr _drawFunc, size_t stackSize)
{
    thrdMgr->newThread(_thrdFunc, _args, _drawFunc, stackSize);
}

ui::progressBar::progressBar(const uint32_t& _max)
{
    max = (float)_max;
    prog = 0;
    width = 0;
}

void ui::progressBar::update(const uint32_t& _prog)
{
    prog = (float)_prog;

    float percent = (float)(prog / max) * 100;
    width  = (float)(percent * 320) / 100;
}

void ui::progressBar::draw()
{
    if (!text.empty()) gfx::drawTextWrap(text, 16, 96, GFX_DEPTH_DEFAULT, 0.5f, 288, 0xFFFFFFFF);

    C2D_DrawRectSolid(0, 207, GFX_DEPTH_DEFAULT, 320, 16, 0x882A2A2A);
    C2D_DrawRectSolid(0, 207, GFX_DEPTH_DEFAULT, width, 16, 0xFFAAAAAA);
    C2D_DrawRectSolid(0, 207, GFX_DEPTH_DEFAULT, width, 1, 0xEEFFFFFF);
}

void ui::progressBarDrawFunc(void *a)
{
    threadInfo *t = (threadInfo *)a;
    if(t->argPtr && t->running) prog->draw();
}

void message_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    ui::confArgs *in = (ui::confArgs *)t->argPtr;
    while(true)
    {
        uint32_t down = ui::padKeysDown();
        ok->update();

        if(down & KEY_A || down & KEY_B || ok->getEvent() == BUTTON_RELEASED)
        {
            if(in->onConfirm)
                ui::newThread(in->onConfirm, in->args, NULL);
            break;
        }
        svcSleepThread(1e+9 / 60);
    }
    t->lock();
    delete in;
    t->argPtr = NULL;
    t->unlock();
    t->finished = true;
}

static void messageDrawFunc(void *a)
{
    threadInfo *t = (threadInfo *)a;
    if(t->argPtr && t->running)
    {
        ui::confArgs *in = (ui::confArgs *)t->argPtr;
        C2D_DrawRectSolid(24, 24, GFX_DEPTH_DEFAULT, 272, 200, gfx::clearClr);
        gfx::drawTextWrap(in->q, 32, 32, GFX_DEPTH_DEFAULT, 0.5f, 256, gfx::txtCont);
        ok->draw();
    }
}

void ui::message(const std::string& mess, funcPtr _onConfirm, void *args)
{
    confArgs *send = new confArgs;
    send->q = mess;
    send->onConfirm = _onConfirm;
    send->args = args;
    ui::newThread(message_t, send, messageDrawFunc);
}

void confirm_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    ui::confArgs *in = (ui::confArgs *)t->argPtr;
    while(true)
    {
        uint32_t down = ui::padKeysDown();
        yes->update();
        no->update();

        if((down & KEY_A || yes->getEvent() == BUTTON_RELEASED))
        {
            if(in->onConfirm)
                ui::newThread(in->onConfirm, in->args, NULL);
            break;
        }
        else if((down & KEY_B || no->getEvent() == BUTTON_RELEASED))
        {
            if(in->onCancel)
                (*in->onCancel)(in->args);
            break;
        }
        svcSleepThread(1e+9 / 60);
    }
    t->lock();
    delete in;
    t->argPtr = NULL;
    t->unlock();
    t->finished = true;
}

static void confirmDrawFunc(void *a)
{
    threadInfo *t = (threadInfo *)a;
    if(t->argPtr && t->running)
    {
        ui::confArgs *in = (ui::confArgs *)t->argPtr;
        C2D_DrawRectSolid(24, 24, GFX_DEPTH_DEFAULT, 272, 200, gfx::clearClr);
        gfx::drawTextWrap(in->q, 32, 32, GFX_DEPTH_DEFAULT, 0.5f, 256, gfx::txtCont);
        yes->draw();
        no->draw();
    }
}

void ui::confirm(const std::string& mess, funcPtr _onConfirm, funcPtr _onCancel, void *args, size_t stackSize)
{
    confArgs *send = new confArgs;
    send->q = mess;
    send->onConfirm = _onConfirm;
    send->onCancel = _onCancel;
    send->args = args;
    ui::newThread(confirm_t, send, confirmDrawFunc, stackSize);
}

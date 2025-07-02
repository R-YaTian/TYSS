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

#ifndef UI_H
#define UI_H

#include <string>
#include <vector>

#include "data.h"
#include "thrdMgr.h"
#include "type.h"

#include "ui/button.h"
#include "ui/menu.h"
#include "ui/ttlview.h"
#include "ui/ttl.h"
#include "ui/ext.h"
#include "ui/sysview.h"
#include "ui/boss.h"
#include "ui/shrd.h"
#include "ui/fld.h"
#include "ui/set.h"

enum states
{
    DAT,
    USR,
    EXT,
    SYS,
    BOS,
    SHR,
    SET
};

enum selop
{
    SEL_NO_OP = 0,
    SEL_BACK_TO_TOP = 1,
    SEL_AUTO = 2
};

extern const std::string TITLE_TEXT;

#ifdef ENABLE_DRIVE
#define FLD_GUIDE_TEXT_DRIVE "\ue000 选择 \ue002 删除 \ue003 恢复 \ue005 上传 \ue001 关闭"
#endif
#define FLD_GUIDE_TEXT "\ue000 选择 \ue002 删除 \ue003 恢复 \ue001 关闭"

namespace ui
{
    enum
    {
        SCREEN_TOP,
        SCREEN_BOT
    };

    typedef struct 
    {
        std::string q;
        funcPtr onConfirm = NULL, onCancel = NULL;
        void *args = NULL;
    } confArgs;
    
    extern uint32_t down, held;
    extern touchPosition p;
    inline void updateInput()
    {
        hidScanInput();
        down = hidKeysDown();
        held = hidKeysHeld();
        touchRead(&p);
    }

    inline uint32_t padKeysDown(){ return down; }
    inline uint32_t padKeysHeld(){ return held; }
    inline touchPosition touchPosition() { return p; }

    void init();
    void exit();
    void showMessage(const char *fmt, ...);
    void newThread(ThreadFunc _thrdFunc, void *_args, funcPtr _drawFunc, size_t stackSize = THRD_STACK_SIZE);
    bool runApp();

    void drawUIBar(const std::string& txt, int screen, bool center);

    class progressBar
    {
        public:
            progressBar() = default;
            progressBar(const uint32_t& _max);
            void setMax(const uint32_t& _max) { max = _max; prog = 0; width = 0; }
            void setText(const std::string& _text) { text = _text; }
            void update(const uint32_t& _prog);
            void draw();

        private:
            float max, prog, width;
            std::string text;
    };

    void progressBarDrawFunc(void *a);
    void confirm(const std::string& mess, funcPtr _onConfirm, funcPtr _onCancel, void *args, size_t stackSize = THRD_STACK_SIZE);
    void message(const std::string& mess, funcPtr _onConfirm, void *args);
    extern const std::string loadGlyphArray[];
    extern int state, prev;
    extern progressBar *prog;
}

#endif // UI_H

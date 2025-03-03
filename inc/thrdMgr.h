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

#include <vector>

#include "type.h"

namespace ui
{
    class threadProcMngr
    {
        public:
            threadProcMngr();
            ~threadProcMngr();

            threadInfo *newThread(ThreadFunc func, void *args, funcPtr _drawFunc, size_t stackSize);
            void update();
            void drawTop();
            void drawBot();
            bool empty() { return threads.empty(); }

        private:
            int cnt = 0, prio = 0;
            std::vector<threadInfo *> threads;
            bool clrAdd = true;
            uint8_t clrShft = 0;
            unsigned frameCount = 0, lgFrame = 0;
            Handle threadLock;
    };
}
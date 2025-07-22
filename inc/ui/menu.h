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

#ifndef MENU_H
#define MENU_H

#include <string>
#include <vector>

#include "type.h"

namespace ui
{
    typedef struct
    {
        funcPtr func = NULL;
        void *args   = NULL;
        uint32_t button = 0;
    } menuOptEvent;

    typedef struct
    {
        std::string txt;
        std::vector<menuOptEvent> events;
    } menuOpt;

    class menu
    {
        public:
            int addOpt(const std::string& add, int maxWidth);
            void addOptEvent(unsigned ind, uint32_t _key, funcPtr _func, void *_args);
            void editOpt(int ind, const std::string& ch){ opt[ind].txt = ch; }
            void reset();
            void adjust();
            void setSelected(const int& newSel);

            void update();
            void draw(const int& x, const int&y, const uint32_t& baseClr, const uint32_t& rectWidth, bool isFileMenu = false);

            int getSelected() { return selected; }
            unsigned getCount() { return opt.size(); }
            std::string getOpt(const int& g) { return opt[g].txt; }

            void setCallback(funcPtr _cb, void *_args) { cb = _cb; args = _args; }

        private:
            uint8_t clrSh = 0x88;
            bool clrAdd = true;
            int selected = 0, start = 0;
            int fc = 0;
            funcPtr cb;
            void *args;
            std::vector<menuOpt> opt;
    };
}

#endif // MENU_H

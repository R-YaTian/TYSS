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
#include "data.h"
#include "type.h"

namespace ui
{
    class titleTile
    {
        public:
            titleTile(bool _fav, C2D_Image *_icon)
            {
                icon = _icon;
                fav  = _fav;
            }

            void draw(int x, int y, bool sel, uint8_t clrShft);

        private:
            C2D_Image *icon;
            bool fav = false;
    };

    class titleview
    {
        public:
            titleview(std::vector<data::titleData>& _t, funcPtr _cb, void *_args);
            ~titleview();

            void update();
            void refresh(std::vector<data::titleData>& _t);
            void draw();
            size_t debGetSize(){ return tiles.size(); }

            void setSelected(int _sel) { selected = _sel; }
            int getSelected() { return selected; }

        private:
            int selected = 0, x = 14, y = 24;
            int selRectX = 0, selRectY = 0;
            bool clrAdd = true;
            uint8_t clrShft = 0;
            std::vector<titleTile> tiles;
            funcPtr callback;
            void *cbArgs;
    };
}

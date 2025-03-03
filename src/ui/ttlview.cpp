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

#include <cmath>
#include <citro2d.h>

#include "gfx.h"
#include "ui.h"
#include "ui/ttlview.h"

void ui::titleTile::draw(int x, int y, bool sel, uint8_t clrShft)
{
    if(sel)
    {
        uint32_t bbClr = 0xFF << 24 | (uint8_t)(0xC5 + (clrShft / 2)) << 16 | (uint8_t)(0x88 + clrShft) << 8 | 0x00;
        gfx::drawBoundingBox(x - 3, y - 3, 54, 54, GFX_DEPTH_DEFAULT, bbClr, false);
    }

    if (icon->subtex->width == 32) {
        C2D_DrawImageAt(gfx::dsIcon(), (float)x, (float)y, GFX_DEPTH_DEFAULT);
        C2D_DrawImageAt(*icon, (float)(x + 8), (float)(y + 8), GFX_DEPTH_DEFAULT);
    } else
        C2D_DrawImageAt(*icon, (float)x, (float)y, GFX_DEPTH_DEFAULT);

    if(fav)
        gfx::drawText("♥", x + 2, y + 2, GFX_DEPTH_DEFAULT, 0.4f, 0xFF4444FF);
}

ui::titleview::titleview(std::vector<data::titleData>& _t, funcPtr _cb, void *args)
{
    callback = _cb;
    cbArgs = args;
    if (_t.empty())
        return;
    for(data::titleData& d : _t)
        tiles.emplace_back(d.getFav(), d.getIcon());
}

ui::titleview::~titleview()
{
    tiles.clear();
}

void ui::titleview::update()
{
    int tileTotal = tiles.size() - 1;

    uint32_t down = ui::padKeysDown();

    switch(down)
    {
        case KEY_DUP:
            if((selected -= 7) < 0)
                selected = 0;
            break;

        case KEY_DDOWN:
            if((selected += 7) > tileTotal)
                selected = tileTotal;
            break;

        case KEY_DLEFT:
            if(--selected < 0)
                selected = 0;
            break;

        case KEY_DRIGHT:
            if(++selected > tileTotal)
                selected = tileTotal;
            break;

        case KEY_L:
            if((selected -= 21) < 0)
                selected = 0;
            break;

        case KEY_R:
            if((selected += 21) > tileTotal)
                selected = tileTotal;
            break;
    }

    selected = tileTotal < 0 ? -1 : selected;

    if(callback)
        (*(callback))(cbArgs);
}

void ui::titleview::refresh(std::vector<data::titleData>& _t)
{
    tiles.clear();
    if (_t.empty())
        return;
    for(data::titleData& d : _t)
        tiles.emplace_back(d.getFav(), d.getIcon());
}

void ui::titleview::draw()
{
    if(tiles.size() <= 0)
        return;

    if(clrAdd && (clrShft += 6) >= 0x72)
        clrAdd = false;
    else if(!clrAdd && (clrShft -= 3) <= 0x00)
        clrAdd = true;

    //Target Y now, temp Y later
    int tY = 176;
    if(selRectY > tY)
    {
        float add = ((float)tY - (float)selRectY) / 3.0f;
        y += ceil(add);
    }
    else if(selRectY < 21)
    {
        float add = (21.0f - (float)selRectY) / 3.0f;
        y += ceil(add);
    }

    int tileTotal = tiles.size();
    for(int tY = y, i = 0; i < tileTotal; tY += 56)
    {
        int endRow = i + 7;
        for(int tX = x; i < endRow; tX += 54, i++)
        {
            if(i >= tileTotal)
                break;

            if(i == selected)
            {
                selRectX = tX - 4;
                selRectY = tY - 4;
                tiles[i].draw(tX, tY, true, clrShft);
            }
            else
                tiles[i].draw(tX, tY, false, 0);
        }
    }
}

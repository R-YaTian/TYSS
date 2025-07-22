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

#include "ui.h"
#include "button.h"
#include "gfx.h"
#include "util.h"

ui::button::button(const std::string &_txt, unsigned _x, unsigned _y, unsigned _w, unsigned _h)
{
    x = _x;
    y = _y;
    w = _w;
    h = _h;
    text = _txt;

    unsigned tw = gfx::getTextWidth(text);
    unsigned th = 14;

    tx = x + (w / 2) - (tw / 2);
    ty = y + (h / 2) - (th / 2);
}

void ui::button::update()
{
    prev = cur;
    cur = ui::getTouchPosition();

    // If button was first thing pressed
    if (isOver() && prev.px == 0 && prev.py == 0)
    {
        first = true;
        pressed = true;
        retEvent = BUTTON_PRESSED;
    }
    else if (retEvent == BUTTON_PRESSED && !util::touchPressed(pos) && wasOver())
    {
        first = false;
        pressed = false;
        retEvent = BUTTON_RELEASED;
    }
    else if (retEvent != BUTTON_NOTHING && !util::touchPressed(pos))
    {
        first = false;
        pressed = false;
        retEvent = BUTTON_NOTHING;
    }
}

bool ui::button::isOver()
{
    return (cur.px > x && cur.px < x + w && cur.py > y && cur.py < y + h);
}

bool ui::button::wasOver()
{
    return (prev.px > x && prev.px < x + w && prev.py > y && prev.py < y + h);
}

void ui::button::draw()
{
    if (pressed)
        C2D_DrawRectSolid(x, y, GFX_DEPTH_DEFAULT, w, h, gfx::btnSel);
    else
        C2D_DrawRectSolid(x, y, GFX_DEPTH_DEFAULT, w, h, gfx::btnClr);

    unsigned _tx = tx;
    unsigned _ty = ty;
    std::string txt = getTextFromMap(text.c_str());
    if (txt != text)
    {
        unsigned tw = gfx::getTextWidth(txt);
        unsigned th = 14;
        _tx = x + (w / 2) - (tw / 2);
        _ty = y + (h / 2) - (th / 2);
    }

    gfx::drawText(txt, _tx, _ty, GFX_DEPTH_DEFAULT, 0.5f, gfx::txtCont);
}

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

#include <string>

#include "ui.h"
#include "gfx.h"
#include "util.h"

int ui::menu::addOpt(const std::string &add, int maxWidth, const std::string& desc)
{
    menuOpt newOpt;
    std::string _desc = "";
    int descWidth = 0;
    int ofs = 2;
    if (util::endsWith(add, std::string(".zip")) || util::endsWith(add, std::string(".sav")) || util::endsWith(add, std::string(".bin")))
        ofs += 3;

    if (!desc.empty() && desc != "")
    {
        _desc = " (" + desc + ")";
        descWidth = gfx::getTextWidth(_desc);
    }

    int strWidth = gfx::getTextWidth(add) + descWidth;
    if (strWidth < maxWidth - 16 || maxWidth == 0)
        newOpt.txt = add + _desc;
    else
    {
        std::u16string tmp, _add = util::toUtf16(add);
        for (unsigned i = 0; i < _add.length(); i++)
        {
            tmp += _add[i];
            if ((int)gfx::getTextWidth(util::toUtf8(tmp)) >= maxWidth - 16 - descWidth)
            {
                tmp.replace(i - ofs, 3, u"···");
                newOpt.txt = util::toUtf8(tmp) + _desc;
                break;
            }
        }
    }

    opt.push_back(newOpt);
    return opt.size() - 1;
}

void ui::menu::addOptEvent(unsigned ind, uint32_t _key, funcPtr _func, void *_args)
{
    menuOptEvent newEvent = {_func, _args, _key};
    opt[ind].events.push_back(newEvent);
}

void ui::menu::reset()
{
    opt.clear();
    clrSh = 0x88;
    clrAdd = true;
    selected = 0;
    fc = 0;
    start = 0;
}

void ui::menu::adjust()
{
    if (selected > (int)opt.size() - 1)
        selected = opt.size() - 1;

    if (opt.size() < 12)
        start = 0;
    else if (opt.size() > 11 && start + 11 > (int)opt.size() - 1)
        start--;
}

void ui::menu::setSelected(const int &newSel)
{
    if (newSel < start || newSel > start + 11)
    {
        int size = opt.size() - 1;
        if (newSel + 11 > size)
            start = size - 11;
        else
            start = newSel;

        selected = newSel;
    }
    else
        selected = newSel;
}

void ui::menu::update()
{
    uint32_t down = ui::padKeysDown();
    uint32_t held = ui::padKeysHeld();

    if ((held & KEY_UP) || (held & KEY_DOWN))
        fc++;
    else
        fc = 0;
    if (fc > 10)
        fc = 0;

    int size = opt.size() - 1;
    if ((down & KEY_UP) || ((held & KEY_UP) && fc == 10))
    {
        selected--;
        if (selected < 0)
            selected = size;

        if (size < 12)
            start = 0;
        else if (start > selected)
            start--;
        else if (selected == size && size > 11)
            start = size - 11;
    }
    else if ((down & KEY_DOWN) || ((held & KEY_DOWN) && fc == 10))
    {
        selected++;
        if (selected > size)
            selected = 0;

        if ((selected > (start + 11)) && ((start + 11) < size))
            start++;
        if (selected == 0)
            start = 0;
    }
    else if (down & KEY_DRIGHT)
    {
        selected += 12;
        if (selected > size)
            selected = size;
        if ((selected - 11) > start)
            start = selected - 11;
    }
    else if (down & KEY_DLEFT)
    {
        selected -= 12;
        if (selected < 0)
            selected = 0;
        if (selected < start)
            start = selected;
    }

    if (down && !opt[selected].events.empty())
    {
        for (ui::menuOptEvent &m : opt[selected].events)
        {
            if (m.func && down & m.button)
                (*(m.func))(m.args);
        }
    }

    if (cb)
        (*(cb))(args);
}

void ui::menu::draw(const int &x, const int &y, const uint32_t &baseClr, const uint32_t &rectWidth, bool isFileMenu)
{
    if (opt.empty())
        return;

    if (clrAdd)
    {
        clrSh += 6;
        if (clrSh >= 0xFA)
            clrAdd = false;
    }
    else
    {
        clrSh -= 3;
        if (clrSh <= 0x88)
            clrAdd = true;
    }

    int length = 0;
    if ((opt.size() - 1) < 12)
        length = opt.size();
    else
        length = start + 12;

    uint32_t rectClr = 0xFF << 24 | 0xC5 << 16 | clrSh << 8 | 0x00;

    for (int i = start; i < length; i++)
    {
        if (i == selected)
        {
            gfx::drawBoundingBox(x, (y - 2) + ((i - start) * 18), rectWidth, 18, GFX_DEPTH_DEFAULT, rectClr);
            C2D_DrawRectSolid(x + 4, y + 1 + ((i - start) * 18), GFX_DEPTH_DEFAULT, 2, 12, 0xFFC5FF00);
        }

        gfx::drawText(isFileMenu ? opt[i].txt : getTextFromMap(opt[i].txt.c_str()), x + 8, (y - 1) + ((i - start) * 18), GFX_DEPTH_DEFAULT, 0.5f, baseClr);
    }
}

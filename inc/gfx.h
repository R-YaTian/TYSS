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

#ifndef GFX_H
#define GFX_H

#include <citro2d.h>
#include <string>
#include "sprites.h"
#include "type.h"

#define GFX_DEPTH_DEFAULT 0.5f

inline C2D_SpriteSheet spritesheet;
inline C2D_Font font = NULL;

namespace gfx
{
    void init();
    void exit();
    void setColor(bool lightback = false);

    extern C3D_RenderTarget *top, *bot;
    extern Tex3DS_SubTexture iconSubTex;
    extern Tex3DS_SubTexture dsIconSubTex;
    extern u32 clearClr, txtCont, rectLt, rectSh, rectSel, divClr, btnClr, btnSel;

    C2D_Image noIcon(void);
    C2D_Image dsIcon(void);

    inline void frameBegin()
    {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, clearClr);
        C2D_TargetClear(bot, clearClr);
    }

    inline void frameEnd()
    {
        C3D_FrameEnd(0);
    }

    inline void frameStartTop()
    {
        C2D_SceneBegin(top);
    }

    inline void frameStartBot()
    {
        C2D_SceneBegin(bot);
    }

    void drawText(const std::string& str, const int& x, const int& y, const float& depth, const float& txtScale, const uint32_t& clr);
    void drawTextWrap(const std::string& str, const int& x, int y, const float& depth, const float& txtScale, const int& maxWidth, const uint32_t& clr);
    void drawU16Text(const std::u16string& str, const int& x, const int& y, const float& depth, const uint32_t& clr);
    size_t getTextWidth(const std::string& str);

    void drawBoundingBox(const int& x, const int& y, const int& w, const int& h, const float& depth, const uint32_t& clr);
}

#endif // GFX_H

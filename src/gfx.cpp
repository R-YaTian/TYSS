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
#include <citro3d.h>
#include <citro2d.h>

#include <string>
#include <cstring>
#include <fstream>

#include "gfx.h"
#include "fs.h"
#include "util.h"

C3D_RenderTarget *gfx::top, *gfx::bot;

//Needed for icon sub tex. Top UV needs to be higher than bottom so it's rotated.
Tex3DS_SubTexture gfx::iconSubTex = {48, 48, 0.0f, 0.75f, 0.75f, 0.0f};
Tex3DS_SubTexture gfx::dsIconSubTex = {32, 32, 0.0f, 1.0f, 1.0f, 0.0f};

u32 gfx::clearClr, gfx::txtCont, gfx::rectLt, gfx::rectSh, gfx::rectSel, gfx::divClr, gfx::btnClr, gfx::btnSel;

C2D_Image gfx::noIcon(void)
{
    return C2D_SpriteSheetGetImage(spritesheet, sprites_noicon_idx);
}

C2D_Image gfx::dsIcon(void)
{
    return C2D_SpriteSheetGetImage(spritesheet, sprites_dsicon_idx);
}

void gfx::setColor(bool light)
{
    clearClr = (light ? 0xFFEFEFEF : 0xFF2D2D2D);
    txtCont = (light ? 0xFF333333 : 0xFFFFFFFF);
    rectLt = (light ? 0xFFDFDFDF : 0xFF505050);
    rectSh = (light ? 0xFFCACACA : 0xFF202020);
    rectSel = (light ? 0xFFF0E8E7 : 0xFF272221);
    divClr = (light ? 0x88555555 : 0x881D1D1D);
    btnClr = (light ? 0xFFDBDBDB : 0xFF545454);
    btnSel = (light ? 0xFFBBBBBB : 0xFF747474);
}

void gfx::init()
{
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    spritesheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
    top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    font = C2D_FontLoad("romfs:/cbf_std.bcfnt");
    font_kor = C2D_FontLoadSystem(CFG_REGION_KOR);
}

void gfx::exit()
{
    C2D_FontFree(font);
    C2D_FontFree(font_kor);
    C2D_SpriteSheetFree(spritesheet);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

void gfx::drawText(const std::string& str, const int& x, const int& y, const float& depth, const float& txtScale, const uint32_t& clr)
{
    C2D_Text tmpTxt;
    C2D_TextBuf tmpBuf = C2D_TextBufNew(512);

    if (font)
        C2D_TextFontParse(&tmpTxt, font, tmpBuf, str.c_str());
    else
        C2D_TextParse(&tmpTxt, tmpBuf, str.c_str());
    C2D_TextOptimize(&tmpTxt);
    C2D_DrawText(&tmpTxt, C2D_WithColor, (float)x, (float)y, depth, txtScale, txtScale, clr);
    C2D_TextBufDelete(tmpBuf);
}

void gfx::drawTextWrap(const std::string& str, const int& x, int y, const float& depth, const float& txtScale, const int& maxWidth, const uint32_t& clr)
{
    if (str.empty()) return;

    C2D_Text tmpTxt;
    C2D_TextBuf tmpBuf = C2D_TextBufNew(512);

    int tmpX = x;
    std::u16string line, word, u16str = util::toUtf16(str);

    auto flushLine = [&](const std::u16string& u16line) {
        if (u16line.empty()) return;
        std::string utf8Line = util::toUtf8(u16line);
        if (font)
            C2D_TextFontParse(&tmpTxt, font, tmpBuf, utf8Line.c_str());
        else
            C2D_TextParse(&tmpTxt, tmpBuf, utf8Line.c_str());
        C2D_TextOptimize(&tmpTxt);
        C2D_DrawText(&tmpTxt, C2D_WithColor, (float)tmpX, (float)y, depth, txtScale, txtScale, clr);
    };

    for (size_t i = 0; i < u16str.size(); i++)
    {
        char16_t ch = u16str[i];

        // Force Linebreak
        if (ch == u'\n')
        {
            if (!word.empty()) {
                line += word;
                word.clear();
            }
            flushLine(line);
            line.clear();
            tmpX = x;
            y += 16;
            continue;
        }

        // Check break char
        bool isBreakChar = (ch == u' ' || ch == u'/' || ch == u'_');

        word.push_back(ch);
        int newWidth = getTextWidth(util::toUtf8(line + word));

        if (tmpX + newWidth > x + maxWidth)
        {
            if (!line.empty())
            {
                flushLine(line);
                tmpX = x;
                y += 16;
                line.clear();
            } else {
                flushLine(word.substr(0, word.size() - 1));
                tmpX = x;
                y += 16;
                word = word.substr(word.size() - 1);
            }
        } else if (isBreakChar) {
            line += word;
            word.clear();
        }
    }

    // Flush remaining
    if (!word.empty()) line += word;
    if (!line.empty()) flushLine(line);

    C2D_TextBufDelete(tmpBuf);
}

void gfx::drawU16Text(const std::u16string& str, const int& x, const int& y, const float& depth, const uint32_t& clr)
{
    C2D_Text tmpTxt;
    C2D_TextBuf tmpBuf = C2D_TextBufNew(512);

    std::string tmp = util::toUtf8(str);

    if (fontHasAllTextChar(font, str))
        C2D_TextFontParse(&tmpTxt, font, tmpBuf, tmp.c_str());
    else
        C2D_TextFontParse(&tmpTxt, font_kor, tmpBuf, tmp.c_str());

    C2D_TextOptimize(&tmpTxt);
    C2D_DrawText(&tmpTxt, C2D_WithColor, (float)x, (float)y, depth, 0.5f, 0.5f, clr);
    C2D_TextBufDelete(tmpBuf);
}

size_t gfx::getTextWidth(const std::string& str)
{
    float ret = 0;
    C2D_Text tmpTxt;
    C2D_TextBuf tmpBuf = C2D_TextBufNew(512);

    if (font)
        C2D_TextFontParse(&tmpTxt, font, tmpBuf, str.c_str());
    else
        C2D_TextParse(&tmpTxt, tmpBuf, str.c_str());
    C2D_TextOptimize(&tmpTxt);

    C2D_TextGetDimensions(&tmpTxt, 0.5f, 0.5f, &ret, NULL);
    C2D_TextBufDelete(tmpBuf);

    return (size_t)ret;
}

void gfx::drawBoundingBox(const int& x, const int& y, const int& w, const int& h, const float& depth, const uint32_t& clr)
{
    C2D_DrawRectSolid(x, y + 1, depth, w, h - 2, rectSel);
    C2D_DrawRectSolid(x + 1, y, depth, w - 2, 2, clr);
    C2D_DrawRectSolid(x, y + 1, depth, 2, h - 2, clr);
    C2D_DrawRectSolid(x + 1, (y + h - 2), depth, w - 2, 2, clr);
    C2D_DrawRectSolid((x + w) - 2, y + 1, depth, 2, h - 2, clr);
}

bool gfx::fontHasAllTextChar(const C2D_Font& font, const std::u16string& str)
{
    const uint16_t* p = reinterpret_cast<const uint16_t*>(str.data());
    const uint16_t* end = p + str.size();

    while (p < end) {
        uint32_t codepoint = 0;
        ssize_t consumed = decode_utf16(&codepoint, p);
        if (consumed <= 0)
            return false;

        if (codepoint != ' ' && !fontHasChar(font, codepoint))
            return false;

        p += consumed;
    }

    return true;
}

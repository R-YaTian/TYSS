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

C2D_Image gfx::noIcon(void)
{
    return C2D_SpriteSheetGetImage(spritesheet, sprites_noicon_idx);
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
}

void gfx::exit()
{
    C2D_FontFree(font);
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
    C2D_Text tmpTxt;
    C2D_TextBuf tmpBuf = C2D_TextBufNew(512);

    int tmpX = x;
    for(int i = 0; i < (int)str.length(); )
    {
        size_t forceBreak = str.find_first_of("\n", i);
        size_t nextBreak = str.find_first_of(" /_.", i);
        if(forceBreak != str.npos && forceBreak < nextBreak)
        {
            std::string temp = str.substr(i, forceBreak - i);
            size_t width = getTextWidth(temp);
            if((int)(tmpX + width) > (x + maxWidth))
            {
                tmpX = x;
                y += 16;
            }
            if (font)
                C2D_TextFontParse(&tmpTxt, font, tmpBuf, temp.c_str());
            else
                C2D_TextParse(&tmpTxt, tmpBuf, temp.c_str());

            C2D_TextOptimize(&tmpTxt);
            C2D_DrawText(&tmpTxt, C2D_WithColor, (float)tmpX, (float)y, depth, txtScale, txtScale, clr);
            tmpX = x;
            y += 16;
            i += temp.length() + 1;
            continue;
        } else if (forceBreak == 0)
        {
            y += 16;
            continue;
        }

        if(nextBreak == str.npos)
        {
            std::string temp = str.substr(i, str.length() - i);
            size_t width = getTextWidth(temp);
            if((int)(tmpX + width) > (x + maxWidth))
            {
                tmpX = x;
                y += 16;
            }
            if (font)
                C2D_TextFontParse(&tmpTxt, font, tmpBuf, str.substr(i, str.length() - i).c_str());
            else
                C2D_TextParse(&tmpTxt, tmpBuf, str.substr(i, str.length() - i).c_str());
            C2D_TextOptimize(&tmpTxt);
            C2D_DrawText(&tmpTxt, C2D_WithColor, (float)tmpX, (float)y, depth, txtScale, txtScale, clr);
            break;
        }
        else
        {
            std::string temp = str.substr(i, (nextBreak + 1) - i);
            size_t width = getTextWidth(temp);
            if((int)(tmpX + width) > (x + maxWidth))
            {
                tmpX = x;
                y += 16;
            }

            if (font)
                C2D_TextFontParse(&tmpTxt, font, tmpBuf, temp.c_str());
            else
                C2D_TextParse(&tmpTxt, tmpBuf, temp.c_str());

            C2D_TextOptimize(&tmpTxt);
            C2D_DrawText(&tmpTxt, C2D_WithColor, (float)tmpX, (float)y, depth, txtScale, txtScale, clr);
            tmpX += width;
            i += temp.length();
        }
    }
    C2D_TextBufDelete(tmpBuf);
}

void gfx::drawU16Text(const std::u16string& str, const int& x, const int& y, const float& depth, const uint32_t& clr)
{
    C2D_Text tmpTxt;
    C2D_TextBuf tmpBuf = C2D_TextBufNew(512);

    std::string tmp = util::toUtf8(str);

    if (font)
        C2D_TextFontParse(&tmpTxt, font, tmpBuf, tmp.c_str());
    else
        C2D_TextParse(&tmpTxt, tmpBuf, tmp.c_str());
    C2D_TextOptimize(&tmpTxt);
    C2D_DrawText(&tmpTxt, C2D_WithColor, (float)x, (float)y, depth, 0.5f, 0.5f, clr);
    C2D_TextBufDelete(tmpBuf);
}

size_t gfx::getTextWidth(const std::string& str)
{
    float ret = 0;
    C2D_Text tmpTxt;
    C2D_TextBuf tmpBuf = C2D_TextBufNew(512);

    C2D_TextParse(&tmpTxt, tmpBuf, str.c_str());
    C2D_TextOptimize(&tmpTxt);

    C2D_TextGetDimensions(&tmpTxt, 0.5f, 0.5f, &ret, NULL);
    C2D_TextBufDelete(tmpBuf);

    return (size_t)ret;
}

void gfx::drawBoundingBox(const int& x, const int& y, const int& w, const int& h, const float& depth, const uint32_t& clr, bool light)
{
    C2D_DrawRectSolid(x, y + 1, depth, w, h - 2, light ? 0xFFFDFDFD : 0xFF272221);
    C2D_DrawRectSolid(x + 1, y, depth, w - 2, 2, clr);
    C2D_DrawRectSolid(x, y + 1, depth, 2, h - 2, clr);
    C2D_DrawRectSolid(x + 1, (y + h - 2), depth, w - 2, 2, clr);
    C2D_DrawRectSolid((x + w) - 2, y + 1, depth, 2, h - 2, clr);
}

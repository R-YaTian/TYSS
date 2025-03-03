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

#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <3ds.h>
#include "data.h"
#include "fs.h"
#include "ui.h"

namespace util
{
    enum datFmt
    {
        DATE_FMT_YMD,
        DATE_FMT_YDM
    };

    std::string toUtf8(const std::u16string& conv);
    std::u16string toUtf16(const std::string& conv);
    std::u16string createPath(data::titleData& dat, const uint32_t& mode);
    std::string getString(const std::string& hint, bool def);
    std::u16string safeString(const std::u16string& s);
    int getInt(const std::string& hint, const int& init, const int& max);
    std::string getDateString(const int& fmt);
    
    void removeLastDirFromString(std::u16string& s);

    void createTitleDir(data::titleData& dat, const uint32_t& mode);

    void copyDirlistToMenu(fs::dirList& d, ui::menu& m);

    bool touchPressed(const touchPosition& p);

    bool fexists(const std::string& path);

    bool endsWith(const std::string& str, const std::string& suffix);

    Result getStepCount(Handle ptmHandle, u16 *stepValue);

    Result setStepCount(Handle ptmHandle, u16 stepValue);

    Result ACU_GetProxyHost(char *host);

    inline void stripChar(char _c, std::string& _s)
    {
        size_t pos = 0;
        while((pos = _s.find(_c)) != _s.npos)
            _s.erase(pos, 1);
    }
}

#endif

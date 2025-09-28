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
#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sys/stat.h>

#include "data.h"

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

    bool touchPressed(const touchPosition& p);

    bool fexists(const std::string& path);

    template <typename CharT>
    inline CharT asciiToLower(CharT ch) {
        return (ch >= CharT('A') && ch <= CharT('Z')) ? ch + CharT(32) : ch;
    }

    template <typename StringT>
    bool endsWith(const StringT& str, const StringT& suffix) {
        using CharT = typename StringT::value_type;

        if (str.size() < suffix.size()) return false;

        size_t offset = str.size() - suffix.size();
        for (size_t i = 0; i < suffix.size(); ++i) {
            CharT c1 = asciiToLower(str[offset + i]);
            CharT c2 = asciiToLower(suffix[i]);
            if (c1 != c2) return false;
        }
        return true;
    }

    template <typename StringT>
    StringT removeSuffix(const StringT& str, const StringT& suffix) {
        if (endsWith(str, suffix)) {
            return str.substr(0, str.size() - suffix.size());
        }
        return str;
    }

    inline void stripChar(char _c, std::string& _s)
    {
        size_t pos = 0;
        while((pos = _s.find(_c)) != _s.npos)
            _s.erase(pos, 1);
    }

    inline long getFileSize(FILE* file) {
        struct stat st;
        int fd = fileno(file);
        if (fstat(fd, &st) != 0) return -1;
        return st.st_size;
    }

    inline std::string formatSize(uint64_t sizeBytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        double size = static_cast<double>(sizeBytes);
        int unitIndex = 0;

        while (size >= 1024.0 && unitIndex < 4) {
            size /= 1024.0;
            ++unitIndex;
        }

        char buf[32];
        if (size - static_cast<int>(size) > 0.0) {
            std::snprintf(buf, sizeof(buf), "%.1f %s", size, units[unitIndex]);
        } else {
            std::snprintf(buf, sizeof(buf), "%d %s", static_cast<int>(size), units[unitIndex]);
        }

        return std::string(buf);
    }
}

#endif

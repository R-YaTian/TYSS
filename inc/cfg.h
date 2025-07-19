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

#include <string>
#include <unordered_map>

#include <3ds/types.h>

namespace cfg
{
    void initToDefault();

    void load();
    void saveCommon();

    extern std::unordered_map<std::string, u8> config;

#ifdef ENABLE_DRIVE
    void saveDrive();
    extern std::string driveClientID, driveClientSecret, driveAuthCode, driveRefreshToken, driveDiskID;
    extern bool driveInitOnBoot;
#endif
}

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

#ifndef SMDH_H
#define SMDH_H

#include <stdint.h>

//Stolen from 3DS HB menu
typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t reserved;
} smdhHeader_s;

typedef struct
{
    uint16_t shortDescription[0x40];
    uint16_t longDescription[0x80];
    uint16_t publisher[0x40];
} smdhTitle_s;

typedef struct
{
    uint8_t gameRatings[0x10];
    uint32_t regionLock;
    uint8_t matchMakerId[0xC];
    uint32_t flags;
    uint16_t eulaVersion;
    uint16_t reserved;
    uint16_t defaultFrame;
    uint32_t cecId;
} smdhSettings_s;

typedef struct
{
    smdhHeader_s header;
    smdhTitle_s applicationTitles[16];
    smdhSettings_s settings;
    uint8_t reserved[0x8];
    uint8_t smallIconData[0x480];
    uint16_t bigIconData[0x900];
} smdh_s;
#endif // SMDH_H

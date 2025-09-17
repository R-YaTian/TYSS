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

#ifndef MISC_H
#define MISC_H

typedef struct ControlApplicationMemoryModeOverrideConfig {
    u32 query : 1; //< Only query the current configuration, do not update it.
    u32 enable_o3ds : 1; //< Enable o3ds memory mode override
    u32 enable_n3ds : 1; //< Enable n3ds memory mode override
    u32 o3ds_mode : 3; //< O3ds memory mode
    u32 n3ds_mode : 3; //< N3ds memory mode
} ControlApplicationMemoryModeOverrideConfig;

namespace misc
{
    void setPC();

    void clearStepHistory(void *a);

    void clearSoftwareLibraryAndPlayHistory(void *a);

    void clearSharedIconCache(void *a);

    void clearHomeMenuIconCache(void *a);

    void resetDemoPlayCount(void *a);

    void clearGameNotes(void *a);

    void removeSoftwareUpdateNag(void *a);

    void unpackWrappedSoftware(void *a);

    void hackStepCount(void *a);

    void rebootToMode3(void *a);
}

#endif

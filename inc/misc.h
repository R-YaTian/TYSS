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
}

#endif

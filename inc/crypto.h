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

/*
 *   This file is part of PKSM-Core
 *   Copyright (C) 2016-2022 Bernardo Giordano, Admiral Fish, piepie62
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
 *       * Requiring preservation of specified reasonable legal notices or
 *         author attributions in that material or in the Appropriate Legal
 *         Notices displayed by works containing it.
 *       * Prohibiting misrepresentation of the origin of that material,
 *         or requiring that modified versions of such material be marked in
 *         reasonable ways as different from the original version.
 */

#ifndef CRYPTO_H
#define CRYPTO_H

#include <array>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <3ds/types.h>
#include "fs.h"

namespace crypto
{
    // This SHA256 implementation is Brad Conte's. It has been modified to have a C++-style
    // interface.
    class SHA256
    {
    private:
        u8 data[64];
        u32 dataLength;
        u64 bitLength;
        std::array<u32, 8> state;

        void update();

    public:
        SHA256() { reinitialize(); }

        void reinitialize()
        {
            dataLength = 0;
            bitLength  = 0;
            state      = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c,
                     0x1f83d9ab, 0x5be0cd19};
        }

        void update(std::span<const u8> buf);
        [[nodiscard]] std::array<u8, 32> finish();
    };

    struct AGBSaveHeader
    {
        u8 magic[4];       // .SAV
        u8 padding1[12];   // Always 0xFF
        u8 cmac[0x10];     // CMAC. MUST BE RECALCULATED ON SAVE
        u8 padding2[0x10]; // Always 0xFF
        u32 contentId;     // Always 1
        u32 savesMade;     // Check this to find which save to load
        u64 titleId;
        u8 sdCid[0x10];
        u32 saveOffset; // Always 0x200
        u32 saveSize;
        u8 padding3[8];      // Always 0xFF
        u8 arm7Registers[8]; // Might be RTC?
        u8 padding4[0x198];  // Get it to the proper size
    };

    [[nodiscard]] std::array<u8, 32> sha256(std::span<const u8> data);
    std::array<u8, 32> calcAGBSaveSHA256(fs::fsfile& file, const AGBSaveHeader& header);
    std::array<u8, 0x10> calcAGBSaveCMAC(
        Handle fspxiHandle, const FSPXI_File& file, const AGBSaveHeader& header, const std::array<u8, 32> hashData);
}

#endif

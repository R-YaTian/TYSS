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

#include <bit>

#include "crypto.h"

#define SHA256_BLOCK_SIZE 32

namespace
{
    inline u32 CH(u32 x, u32 y, u32 z)
    {
        return (x & y) ^ (~x & z);
    }

    inline u32 MAJ(u32 x, u32 y, u32 z)
    {
        return (x & y) ^ (x & z) ^ (y & z);
    }

    inline u32 EP0(u32 x)
    {
        return std::rotr(x, 2) ^ std::rotr(x, 13) ^ std::rotr(x, 22);
    }

    inline u32 EP1(u32 x)
    {
        return std::rotr(x, 6) ^ std::rotr(x, 11) ^ std::rotr(x, 25);
    }

    inline u32 SIG0(u32 x)
    {
        return std::rotr(x, 7) ^ std::rotr(x, 18) ^ (x >> 3);
    }

    inline u32 SIG1(u32 x)
    {
        return std::rotr(x, 17) ^ std::rotr(x, 19) ^ (x >> 10);
    }
}

namespace crypto
{
    namespace internal
    {
        constexpr u32 sha256_table[64] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
            0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be,
            0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
            0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
            0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e,
            0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624,
            0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3,
            0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
            0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
    }

    std::array<u8, 32> sha256(std::span<const u8> buf)
    {
        SHA256 context;
        context.update(buf);
        return context.finish();
    }

    void SHA256::update()
    {
        uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

        for (i = 0, j = 0; i < 16; ++i, j += 4)
        {
            m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
        }
        for (; i < 64; ++i)
        {
            m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
        }

        a = state[0];
        b = state[1];
        c = state[2];
        d = state[3];
        e = state[4];
        f = state[5];
        g = state[6];
        h = state[7];

        for (i = 0; i < 64; ++i)
        {
            t1 = h + EP1(e) + CH(e, f, g) + internal::sha256_table[i] + m[i];
            t2 = EP0(a) + MAJ(a, b, c);
            h  = g;
            g  = f;
            f  = e;
            e  = d + t1;
            d  = c;
            c  = b;
            b  = a;
            a  = t1 + t2;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

    void SHA256::update(std::span<const u8> buf)
    {
        for (size_t i = 0; i < buf.size(); i++)
        {
            data[dataLength++] = buf[i];
            if (dataLength == 64)
            {
                update();
                bitLength  += 512;
                dataLength = 0;
            }
        }
    }

    std::array<u8, 32> SHA256::finish()
    {
        std::array<u8, 32> ret;

        uint32_t i = dataLength;

        // Pad whatever data is left in the buffer.
        if (dataLength < 56)
        {
            data[i++] = 0x80;
            while (i < 56)
            {
                data[i++] = 0x00;
            }
        }
        else
        {
            data[i++] = 0x80;
            while (i < 64)
            {
                data[i++] = 0x00;
            }
            update();
            std::fill_n(data, 56, 0);
        }

        // Append to the padding the total message's length in bits and transform.
        bitLength += dataLength * 8;
        data[63]  = bitLength;
        data[62]  = bitLength >> 8;
        data[61]  = bitLength >> 16;
        data[60]  = bitLength >> 24;
        data[59]  = bitLength >> 32;
        data[58]  = bitLength >> 40;
        data[57]  = bitLength >> 48;
        data[56]  = bitLength >> 56;
        update();

        // Since this implementation uses little endian byte ordering and SHA uses big endian,
        // reverse all the bytes when copying the final state to the output hash.
        for (i = 0; i < 4; ++i)
        {
            ret[i]      = (state[0] >> (24 - i * 8)) & 0x000000ff;
            ret[i + 4]  = (state[1] >> (24 - i * 8)) & 0x000000ff;
            ret[i + 8]  = (state[2] >> (24 - i * 8)) & 0x000000ff;
            ret[i + 12] = (state[3] >> (24 - i * 8)) & 0x000000ff;
            ret[i + 16] = (state[4] >> (24 - i * 8)) & 0x000000ff;
            ret[i + 20] = (state[5] >> (24 - i * 8)) & 0x000000ff;
            ret[i + 24] = (state[6] >> (24 - i * 8)) & 0x000000ff;
            ret[i + 28] = (state[7] >> (24 - i * 8)) & 0x000000ff;
        }

        return ret;
    }

    // On return, will be at the end of the save described by the header.
    std::array<u8, 32> calcAGBSaveSHA256(fs::fsfile& file, u32 saveSize)
    {
        static constexpr size_t READBLOCK_SIZE = 0x1000;

        SHA256 context;
        file.seek(0x30, fs::fsSeek::seek_beg);
        size_t sha_end_idx              = saveSize + 0x200 - 0x30;
        std::unique_ptr<u8[]> readblock = std::unique_ptr<u8[]>(new u8[READBLOCK_SIZE]);
        size_t read                     = 0;
        for (size_t i = 0; i < sha_end_idx; i += read)
        {
            size_t readSize = std::min(sha_end_idx - i, READBLOCK_SIZE);
            read            = file.read(readblock.get(), readSize);
            context.update({readblock.get(), readSize});
        }

        return context.finish();
    }

    // Who the hell came up with this shit? Nintendo, please fire whatever employee thought this
    // was a good idea CMAC = AES-CMAC(SHA256("CTR-SIGN" + titleID + SHA256("CTR-SAV0" +
    // SHA256(0x30..0x200 + the entire save itself)))) FSPXI_CalcSavegameMAC does the AES-CMAC,
    // CTR-SIGN, and the CTR-SAV0 step
    std::array<u8, 0x10> calcAGBSaveCMAC(
        Handle fspxiHandle, const FSPXI_File& file, const std::array<u8, 32> hashData)
    {
        std::array<u8, 0x10> prev;
        std::array<u8, 0x10> ret;

        int tries = 10;
        Result res;
        do
        {
            --tries;
            prev = ret;
            res  = FSPXI_CalcSavegameMAC(fspxiHandle, file,
                 hashData.data(), hashData.size(), ret.data(), ret.size());
        }
        while (R_SUCCEEDED(res) && prev != ret && tries > 0);

        return ret;
    }
}

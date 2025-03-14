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
 *   This file is part of Checkpoint
 *   Copyright (C) 2017-2021 Bernardo Giordano, FlagBrew
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

#include "util.h"
#include "cheatmanager.h"

CheatManager::CheatManager(void)
{
    mCheats = nullptr;
    if (util::fexists("/TYSS/cheats.json")) {
        const std::string path = "/TYSS/cheats.json";
        FILE* in               = fopen(path.c_str(), "rt");
        if (in != NULL) {
            mCheats = std::make_shared<nlohmann::ordered_json>(nlohmann::ordered_json::parse(in, nullptr, false));
            fclose(in);
        }
    } else {
        const std::string path = "romfs:/cheats/cheats.json.bz2";
        // load compressed archive in memory
        FILE* f = fopen(path.c_str(), "rb");
        if (f != NULL) {
            fseek(f, 0, SEEK_END);
            u32 size             = ftell(f);
            unsigned int destLen = CHEAT_SIZE_DECOMPRESSED;
            char* s              = new char[size];
            char* d              = new char[destLen + 1]();
            rewind(f);
            fread(s, 1, size, f);

            int r = BZ2_bzBuffToBuffDecompress(d, &destLen, s, size, 0, 0);
            if (r == BZ_OK) {
                mCheats = std::make_shared<nlohmann::ordered_json>(nlohmann::ordered_json::parse(d));
            }

            delete[] s;
            delete[] d;
            fclose(f);
        }
    }
}

bool CheatManager::areCheatsAvailable(const std::string& key)
{
    return mCheats->find(key) != mCheats->end();
}

bool CheatManager::install(const std::string& key)
{
    std::string cheatFile = "";
    auto cheats           = *CheatManager::getInstance().cheats().get();
    bool first            = true;
    for (auto it = cheats[key].begin(); it != cheats[key].end(); ++it) {
        std::string value = it.key();
        if (!first)
            cheatFile += "\n";
        cheatFile += "[" + value + "]\n";
        for (auto& code : cheats[key][value]) {
            cheatFile += code.get<std::string>() + "\n";
        }

        if (first)
            first = false;
    }

    const std::string outPath = "/cheats/" + key + ".txt";
    FILE* f                   = fopen(outPath.c_str(), "w");
    if (f != NULL) {
        fwrite(cheatFile.c_str(), 1, cheatFile.length(), f);
        fclose(f);
        return true;
    }

    return false;
}
